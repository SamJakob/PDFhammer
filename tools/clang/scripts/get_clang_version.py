#!/usr/bin/env python3

from typing import Optional, Final

import os
import sys
import re

import argparse
import subprocess

DEFAULT_CLANG_PATH: Final[str] = "/usr/bin/clang"
"""The default path to use for clang."""

def get_clang_version(clang_path: Optional[str] = DEFAULT_CLANG_PATH,
                      get_full_version: bool = False) -> Optional[str]:
    """
    Effectively invokes `clang --version` where clang is the clang_path binary.
    If clang_path is None or blank, `DEFAULT_CLANG_PATH` is used.

    If the clang version cannot be resolved, this function returns None.
    """

    version_string = subprocess.check_output([ clang_path, "--version" ], env=os.environ).decode('utf-8')
    version_expr = re.compile(r'clang version ((\d+)\.\d+\.\d+)', re.IGNORECASE)
    version_result = version_expr.search(version_string)

    if version_result is not None:
        return version_result.group(1) if get_full_version else version_result.group(2)
    else:
        return None

def main() -> int:
    parser = argparse.ArgumentParser(description='Get information about the currently instance of clang.')
    parser.add_argument('--clang-path',
                        help=f'The path to the clang executable (default: {DEFAULT_CLANG_PATH})',
                        default=DEFAULT_CLANG_PATH,
                        action='store')
    parser.add_argument('--full-version',
                        help='Print the full version instead of just the major version (default: false)',
                        default=False,
                        action='store_true')
    args = parser.parse_args()
    clang_path = args.clang_path

    version = get_clang_version(clang_path, args.full_version)
    if version is None:
        print("Failed to parse clang version string.", file=sys.stderr)
        return 1

    # Print the appropriate match depending on whether full-version is set.
    print(version,
          file=sys.stdout)

    return 0

if __name__ == '__main__':
    sys.exit(main())
