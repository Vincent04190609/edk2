"""Shared path resolution for bios-engineering-spec scripts."""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def load_resolver():
    script = repo_root() / ".cursor" / "scripts" / "resolve-kb-paths.py"
    spec = importlib.util.spec_from_file_location("resolve_kb_paths", script)
    if spec is None or spec.loader is None:
        raise ImportError(f"Cannot load resolver from {script}")
    module = importlib.util.module_from_spec(spec)
    sys.modules["resolve_kb_paths"] = module
    spec.loader.exec_module(module)
    return module


def engineering_spec_path(override: str | None = None) -> Path:
    if override:
        return Path(override)
    resolver = load_resolver()
    resolved = resolver.resolve_paths(resolver.load_config())
    return Path(resolved["ENGINEERING_SPEC_DOC"])
