---
name: bios-engineering-spec
description: >-
  Read and update the project Engineering Spec Word document (.docx) when adding or
  modifying BIOS features (SMBIOS, Setup menu, HII, PCD-driven options, customer-specific
  features). Use when implementing or changing firmware-visible behavior, when the user
  mentions Engineering Spec, or when feature work requires spec/documentation sync.
  Resolves path via resolve-kb-paths.py; default file is Engineering Spec.docx under
  PROJECT_KB_ROOT/development-guides/playbooks/.
---

# BIOS Engineering Spec (Word)

Keeps **`Engineering Spec.docx`** aligned with firmware feature work. Run alongside the **knowledge-capture** skill (markdown playbook/runbook) — the Word spec is the formal engineering document; playbooks are agent/engineer how-to notes.

## Path resolution

1. Run **`python .cursor/scripts/resolve-kb-paths.py`** → use **`ENGINEERING_SPEC_DOC`**
2. Override filename per project in **`.cursor/kb-path-config.json`** → `engineering_spec_relative_path`

Default: `{PROJECT_KB_ROOT}/development-guides/playbooks/Engineering Spec.docx`

> **Out-of-workspace write**: same as KB playbooks — file lives outside edk2. Use **elevated permissions** for save, then **re-read** to verify.

## Dependencies

Install once (Python 3.10+):

```bash
pip install -r .cursor/skills/bios-engineering-spec/requirements.txt
```

Requires **`python-docx`**. For complex formatting or tracked changes, fall back to the xlsx skill Office tools: `.cursor/skills/xlsx/scripts/office/unpack.py` and `pack.py`.

## When to run (mandatory for feature work)

Update Engineering Spec when the change affects **documented product behavior**, including:

| Area | Spec section | Example update |
|------|--------------|----------------|
| SMBIOS | Customer Specific Features → System Management BIOS | Type 1 field name/default in table |
| Setup menu | SETUP MENU → Setting Items | New HII item + default/current setting |
| Customer features | Customer Specific Features | New subsection for ACPI EC, POST message, etc. |
| Visible defaults | SMBIOS or Setup tables | Default value, token name, PCD reference |

**Skip** for pure refactors, build fixes, or internal changes with no spec impact.

## Document structure (Onboarding template)

Current **`Engineering Spec.docx`** layout:

| Section | Content |
|---------|---------|
| Header | `Version: x.y`, `Date: ...` |
| Customer Specific Features | SMBIOS Type 1/2/3 labels + **Name / Default Value** table |
| SETUP MENU | **Items / Current Setting** table |

Read live structure before editing:

```bash
python .cursor/skills/bios-engineering-spec/scripts/read-engineering-spec.py
```

## Workflow

### Step 1 — Read before change

```bash
python .cursor/skills/bios-engineering-spec/scripts/read-engineering-spec.py
```

Identify the target section (SMBIOS table, Setup table, or heading to extend).

### Step 2 — Implement firmware change

Complete code/HII/PCD/SMBIOS changes in edk2 first (or in parallel), using values you will record in the spec.

### Step 3 — Update the Word spec

**Preferred — helper script** (table rows + metadata):

```bash
# Bump Version x.y and Date (do once per spec edit session)
python .cursor/skills/bios-engineering-spec/scripts/update-engineering-spec.py --bump-metadata

# SMBIOS Type 1 table (Name / Default Value)
python .cursor/skills/bios-engineering-spec/scripts/update-engineering-spec.py \
  --smbios-field "System Serial Number" "987654321"

# Setup menu table (Items / Current Setting)
python .cursor/skills/bios-engineering-spec/scripts/update-engineering-spec.py \
  --setup-item "Enable Turbo" "Enabled"
```

Combine flags in one invocation when possible.

**Manual — python-docx** when adding headings, new tables, or Type 2/3 sections:

```python
from docx import Document

doc = Document(r"<ENGINEERING_SPEC_DOC>")
# Add Heading 2, paragraphs, or new tables under Customer Specific Features
doc.save(r"<ENGINEERING_SPEC_DOC>")
```

Preserve existing styles (`Heading 1`, `Heading 2`, `List Paragraph`, table headers).

**Advanced — OOXML unpack/edit/pack** (preserve tracked changes / strict layout):

```bash
python .cursor/skills/xlsx/scripts/office/unpack.py "<ENGINEERING_SPEC_DOC>" /tmp/spec-unpacked/
# edit word/document.xml or related parts
python .cursor/skills/xlsx/scripts/office/pack.py /tmp/spec-unpacked/ "<ENGINEERING_SPEC_DOC>" \
  --original "<ENGINEERING_SPEC_DOC>"
python .cursor/skills/xlsx/scripts/office/validate.py "<ENGINEERING_SPEC_DOC>" --original "<ENGINEERING_SPEC_DOC>"
```

### Step 4 — Verify

1. Re-run **`read-engineering-spec.py`** and confirm rows/values.
2. Optionally open the `.docx` on Windows to visually check tables.
3. Report path and summary to the user.

## Content checklist per feature type

### SMBIOS

- [ ] Type number (1, 2, 3, …) noted in spec text if new type added
- [ ] Each exposed field: **Name** + **Default Value** (or "N/A" / "Platform")
- [ ] Matches built firmware (QEMU dump, smbiosview, or project test script)

### Setup menu

- [ ] Form set / menu path (e.g. Advanced → CPU Configuration)
- [ ] Item label matches HII string
- [ ] **Current Setting** default matches PCD/token default
- [ ] Options documented if not boolean (Enabled/Disabled/Auto)

### General

- [ ] `--bump-metadata` applied when spec content changed
- [ ] Issue ID or BIOS version referenced in agent summary if applicable

## Hand off to user

```markdown
## Engineering Spec updated

| Field | Value |
|-------|-------|
| Path | `{ENGINEERING_SPEC_DOC}` |
| Version | x.y |
| Sections | SMBIOS Type 1, Setup menu, … |

**Summary**: [1-2 sentences tying spec rows to firmware change]

Please review in Word before committing the knowledge-base repo.
```

Emit with playbook write-back when feature work completes:

```
KB_UPDATED: yes — {playbook path}, {ENGINEERING_SPEC_DOC}
```

## Related

- `.cursor/skills/knowledge-capture/SKILL.md` — markdown playbook/runbook
- `.cursor/skills/bios-release-version/SKILL.md` — VersionList.xlsx (separate from spec version)
- `.cursor/rules/knowledge-capture.mdc` — enforcement
- `.cursor/KB-PATH-MODES.md` — fleet vs windows KB paths
