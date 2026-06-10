#!/usr/bin/env python3
"""Read Engineering Spec.docx structure as JSON for agent planning."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from docx import Document
from docx.document import Document as DocxDocument
from docx.oxml.ns import qn
from docx.table import Table
from docx.text.paragraph import Paragraph

from spec_paths import engineering_spec_path


def iter_block_items(parent: DocxDocument):
    for child in parent.element.body.iterchildren():
        if child.tag == qn("w:p"):
            yield Paragraph(child, parent)
        elif child.tag == qn("w:tbl"):
            yield Table(child, parent)


def table_to_rows(table: Table) -> list[list[str]]:
    return [[cell.text.strip() for cell in row.cells] for row in table.rows]


def read_spec(path: Path) -> dict:
    doc = Document(str(path))
    blocks: list[dict] = []
    current_h1 = ""
    current_h2 = ""

    for block in iter_block_items(doc):
        if isinstance(block, Paragraph):
            text = block.text.strip()
            if not text:
                continue
            style = block.style.name if block.style else "Normal"
            entry = {"kind": "paragraph", "style": style, "text": text}
            if style == "Heading 1":
                current_h1 = text
                current_h2 = ""
            elif style == "Heading 2":
                current_h2 = text
            entry["section_h1"] = current_h1
            entry["section_h2"] = current_h2
            blocks.append(entry)
        else:
            blocks.append(
                {
                    "kind": "table",
                    "section_h1": current_h1,
                    "section_h2": current_h2,
                    "rows": table_to_rows(block),
                }
            )

    metadata = {}
    for block in blocks:
        if block["kind"] != "paragraph":
            continue
        text = block["text"]
        if text.startswith("Version:"):
            metadata["version"] = text.split(":", 1)[1].strip()
        elif text.startswith("Date:"):
            metadata["date"] = text.split(":", 1)[1].strip()

    return {
        "path": str(path),
        "metadata": metadata,
        "blocks": blocks,
        "feature_sections": {
            "smbios": [
                b
                for b in blocks
                if b.get("section_h1") == "Customer Specific Features"
                or "SMBIOS" in b.get("text", "")
                or (
                    b.get("kind") == "table"
                    and b.get("section_h1") == "Customer Specific Features"
                )
            ],
            "setup_menu": [
                b
                for b in blocks
                if b.get("section_h1") == "SETUP MENU"
                or b.get("section_h2") == "Setting Items"
            ],
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Read Engineering Spec.docx outline.")
    parser.add_argument(
        "--path",
        help="Override path (default: resolved ENGINEERING_SPEC_DOC)",
    )
    args = parser.parse_args()

    try:
        spec_path = engineering_spec_path(args.path)
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return 1

    if not spec_path.is_file():
        print(
            f"Engineering Spec not found: {spec_path}\n"
            "Hint: set mode to 'windows' in .cursor/kb-path-config.json on native Windows, "
            "or ensure Fleet bind mounts are active for fleet mode.",
            file=sys.stderr,
        )
        return 1

    print(json.dumps(read_spec(spec_path), indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
