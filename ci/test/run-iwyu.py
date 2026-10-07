#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import json
import os
import re
import runpy
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


FILES_WITH_ENFORCED_IWYU = re.compile(
    r"/src/((bench|common|consensus|crypto|index|init|kernel|primitives|rpc|script|univalue/(lib|test)|util|zmq)/.*"
    r"|node/(blockstorage|interfaces|miner|mining_args|utxo_snapshot)"
    r"|test/fuzz/(kitchen_sink|minisketch|parse_univalue)"
    r"|clientversion|core_io|rest|signet|init)\.cpp"
)


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as error:
        sys.exit(str(error))


def subtree_pattern(root: Path) -> re.Pattern[str]:
    subtrees = runpy.run_path(str(root / "test/lint/lint_ignore_dirs.py"))["SHARED_EXCLUDED_SUBTREES"]
    return re.compile("|".join(re.escape(path) for path in subtrees))


def run_iwyu(root: Path, build_dir: Path, compile_commands: str, makejobs: str, llvm_version: str):
    shutil.move(build_dir / compile_commands, build_dir / "compile_commands.json")
    clang_resource_dir = run(
        [f"clang-{llvm_version}", "-print-resource-dir"],
        stdout=subprocess.PIPE,
        text=True,
    ).stdout.strip()
    run(
        [
            "python3",
            "/include-what-you-use/mapgen/iwyu-mapgen-clang-intrin.py",
            "--lang",
            "imp",
            f"{clang_resource_dir}/include",
        ],
        stdout=(build_dir / "clang.intrinsics.imp").open("w"),
    )

    iwyu_output = run(
        [
            "python3",
            "/include-what-you-use/iwyu_tool.py",
            "-p",
            str(build_dir),
            makejobs,
            "--",
            "-Xiwyu",
            "--cxx17ns",
            "-Xiwyu",
            f"--mapping_file={root / 'contrib/devtools/iwyu/bitcoin.core.imp'}",
            "-Xiwyu",
            f"--mapping_file={build_dir / 'clang.intrinsics.imp'}",
            "-Xiwyu",
            "--max_line_length=160",
            "-Xiwyu",
            r"--check_also=*/common/types\.h",
            "-Xiwyu",
            r"--check_also=*/consensus/*\.h",
            "-Xiwyu",
            r"--check_also=*/interfaces/*\.h",
            "-Xiwyu",
            r"--check_also=*/primitives/transaction_identifier\.h",
            "-Xiwyu",
            r"--check_also=*/rpc/protocol\.h",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    Path("/tmp/iwyu_ci.out").write_text(iwyu_output.stdout)
    print(iwyu_output.stdout, end="")
    with Path("/tmp/iwyu_ci.out").open() as output:
        run(
            ["python3", "/include-what-you-use/fix_includes.py", "--nosafe_headers"],
            stdin=output,
        )

    # Subtree sources are already filtered out of the compilation database.
    # IWYU can still edit a subtree header it treats as associated with a
    # source file outside the subtree (minisketch.h for test/fuzz/minisketch.cpp).
    excluded_subtrees = runpy.run_path(str(root / "test/lint/lint_ignore_dirs.py"))["SHARED_EXCLUDED_SUBTREES"]
    run(["git", "restore", "--", *excluded_subtrees])
    diff = run(["git", "diff", "-U1"], stdout=subprocess.PIPE)
    run(
        ["./contrib/devtools/clang-format-diff.py", f"-binary=clang-format-{llvm_version}", "-p1", "-i", "-v"],
        input=diff.stdout,
    )


def main():
    root = Path(os.environ["BASE_ROOT_DIR"])
    os.chdir(root)
    build_dir = Path(os.environ["BASE_BUILD_DIR"])
    excluded = subtree_pattern(root)
    all_compile_commands = [
        entry for entry in json.loads((build_dir / "compile_commands.json").read_text())
        if not excluded.search(entry["file"])
    ]
    (build_dir / "compile_commands_iwyu_errors.json").write_text(
        json.dumps([entry for entry in all_compile_commands if FILES_WITH_ENFORCED_IWYU.search(entry["file"])])
    )
    (build_dir / "compile_commands_iwyu_warnings.json").write_text(
        json.dumps([entry for entry in all_compile_commands if not FILES_WITH_ENFORCED_IWYU.search(entry["file"])])
    )

    makejobs = os.environ["MAKEJOBS"]
    llvm_version = os.environ["IWYU_LLVM_V"]
    run_iwyu(root, build_dir, "compile_commands_iwyu_errors.json", makejobs, llvm_version)
    if run(["git", "diff", "--exit-code"], check=False).returncode:
        print("^^^ ⚠️ Failure generated from IWYU")
        raise SystemExit(1)

    run_iwyu(root, build_dir, "compile_commands_iwyu_warnings.json", makejobs, llvm_version)
    run(["git", "--no-pager", "diff"])


if __name__ == "__main__":
    main()
