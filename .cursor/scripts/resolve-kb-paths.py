#!/usr/bin/env python3
"""Resolve knowledge-base paths from .cursor/kb-path-config.json.

Agent-agnostic: Cursor rules, skills, hooks, and other agents should run this
script (or read its JSON output) instead of hardcoding d:\\ or /mnt/d paths.

Usage:
  python .cursor/scripts/resolve-kb-paths.py
  python .cursor/scripts/resolve-kb-paths.py --format shell
  python .cursor/scripts/resolve-kb-paths.py --key PROJECT_KB_ROOT
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def config_path() -> Path:
    return repo_root() / ".cursor" / "kb-path-config.json"


def load_config() -> dict:
    path = config_path()
    if not path.is_file():
        raise FileNotFoundError(f"KB path config not found: {path}")
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def normalize_slashes(path: str) -> str:
    return path.replace("\\", "/")


def join_path(root: str, *parts: str) -> str:
    root = root.rstrip("\\/")
    segments = [part.strip("\\/") for part in parts if part]
    if re.match(r"^[A-Za-z]:", root):
        joined = root + "\\" + "\\".join(segments)
        return joined
    return "/".join([root, *segments])


def windows_to_wsl(path: str) -> str:
    normalized = normalize_slashes(path)
    match = re.match(r"^([A-Za-z]):/(.*)$", normalized)
    if not match:
        return normalized
    drive = match.group(1).lower()
    remainder = match.group(2)
    if remainder:
        return f"/mnt/{drive}/{remainder}"
    return f"/mnt/{drive}"


def running_on_windows() -> bool:
    return os.name == "nt"


def path_exists(path: str) -> bool:
    try:
        return Path(path).exists()
    except OSError:
        return False


def resolve_runtime_path(path: str, mode: str) -> str:
    if running_on_windows() or mode == "fleet":
        return path
    if path_exists(path):
        return path
    wsl_path = windows_to_wsl(path)
    if path_exists(wsl_path):
        return wsl_path
    return wsl_path


def resolve_paths(config: dict) -> dict:
    mode = config.get("mode", "fleet")
    project_name = config["project_name"]
    paths = config["paths"]

    if mode not in ("fleet", "windows"):
        raise ValueError(f"Unsupported mode '{mode}'. Use 'fleet' or 'windows'.")

    if mode == "fleet":
        fleet_paths = paths["fleet"]
        project_kb_root = join_path(
            fleet_paths["project_kb_projects_root"], project_name
        )
        common_kb_root = fleet_paths["common_kb_root"]
    else:
        windows_paths = paths["windows"]
        project_kb_root = join_path(
            windows_paths["project_kb_projects_root"], project_name
        )
        common_kb_root = windows_paths["common_kb_root"]
        project_kb_root = resolve_runtime_path(project_kb_root, mode)
        common_kb_root = resolve_runtime_path(common_kb_root, mode)

    project_kb_root = normalize_slashes(project_kb_root)
    common_kb_root = normalize_slashes(common_kb_root)
    project_kb_readme = join_path(project_kb_root, "README.md")
    project_kb_readme = normalize_slashes(project_kb_readme)

    code_repo = config.get("code_repo", {})
    code_repo_root = code_repo.get("root", "")
    if code_repo_root and mode == "windows":
        code_repo_root = normalize_slashes(
            resolve_runtime_path(code_repo_root, mode)
        )
    elif code_repo_root:
        code_repo_root = normalize_slashes(code_repo_root)

    spec_relative = config.get(
        "engineering_spec_relative_path",
        "development-guides/playbooks/Engineering Spec.docx",
    )
    engineering_spec_doc = normalize_slashes(
        join_path(project_kb_root, spec_relative)
    )

    return {
        "mode": mode,
        "project_name": project_name,
        "PROJECT_KB_ROOT": project_kb_root,
        "COMMON_KB_ROOT": common_kb_root,
        "PROJECT_KB_README": project_kb_readme,
        "ENGINEERING_SPEC_DOC": engineering_spec_doc,
        "COMMON_FROM_PROJECT": "../../Common",
        "CODE_REPO_ROOT": code_repo_root,
        "CODE_REPO_URL": code_repo.get("url", ""),
        "config_file": normalize_slashes(str(config_path())),
        "outside_workspace": True,
        "write_requires_elevated_permissions": True,
    }


def format_shell(resolved: dict) -> str:
    lines = []
    for key, value in resolved.items():
        if isinstance(value, bool):
            lines.append(f"{key}={'true' if value else 'false'}")
        else:
            escaped = str(value).replace('"', '\\"')
            lines.append(f'{key}="{escaped}"')
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Resolve KB paths for agents.")
    parser.add_argument(
        "--format",
        choices=("json", "shell"),
        default="json",
        help="Output format (default: json)",
    )
    parser.add_argument(
        "--key",
        help="Print a single resolved key instead of the full object",
    )
    args = parser.parse_args()

    try:
        resolved = resolve_paths(load_config())
    except (FileNotFoundError, ValueError, KeyError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    if args.key:
        if args.key not in resolved:
            print(f"Unknown key: {args.key}", file=sys.stderr)
            return 1
        print(resolved[args.key])
        return 0

    if args.format == "shell":
        print(format_shell(resolved))
    else:
        print(json.dumps(resolved, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
