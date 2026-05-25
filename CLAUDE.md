# BIOS Project Documentation

This file contains codebase context and development guidelines for the BIOS project.

## Knowledge Base

### External Knowledge Base

**Primary Knowledge Repository**: `d:\VibeCoding\Projects\Onboarding\README.md`

This external knowledge base contains comprehensive BIOS/UEFI development documentation:
\\
Note: Relative paths (../../) in spec references are relative to the Primary Knowledge Repository (d:\VibeCoding\Projects\Onboarding\), not the current source tree.
\\


- **BIOS/UEFI Reference** (`../../Common/Industry-Standard-Documentation/UEFI-Spec/UEFI.md`) - UEFI Technical specifications, protocols, and implementation details

- **SMBIOS Spec** (`../../Common/Industry-Standard-Documentation/SMBIOS-Spec/SMBIOS.md`) - SMBIOS Technical specifications, protocols, and implementation details

- **ACPI Spec** (`../../Common/Industry-Standard-Documentation/ACP-Spec/ACPI.md`) - ACPI Technical specifications, protocols, and implementation details

- **Development Guides** (`./development-guides/`) - Coding standards,Release BIOS rules, Commit rule, build processes, and best practices
- **Architecture** (`./architecture/`) - System architecture, design decisions, and component relationships
- **Troubleshooting** (`./troubleshooting/`) - Common issues, debugging techniques, and solutions
- **Onboarding** (`./onboarding/`) - Getting started materials, environment setup, and first-build guides

**Quick Access**: For specific guidance, read `d:\VibeCoding\Projects\Onboarding\README.md` and navigate to relevant subdirectories.

### Project Knowledge Base (docs/)

The project also maintains local knowledge in the `docs/` directory:

- **docs/processes/** - Development workflows and procedures (CI/CD setup, code review protocol)
- **docs/architecture/** - Infrastructure and system architecture documentation
- **docs/security/** - Security review processes and audit procedures

See [docs/README.md](docs/README.md) for complete local knowledge base structure.

### Documentation Guidelines

**Knowledge Base Files** (store in `docs/`):
- Setup guides and procedures
- Collaboration protocols
- Architecture references
- Security review processes

**Temporary Status Files** (working directories only):
- Build status tracking (BUILD_*.md)
- Issue-specific reports (FWB-*_*.md)
- Blocked status (BLOCKED.md)

**Framework Documentation** (keep with code):
- EDK2 README files
- Tooling docs (.pytool, .azurepipelines)

## Knowledge Storage Guidance

### Repository Setup

**Critical**: Configure git to handle line ending differences between Windows and Ubuntu environments.

#### Clone Strategy

```bash
# Configure git to preserve line endings (prevents CR issues between Windows/Ubuntu)
git config --global core.autocrlf false

# Clone with submodules
git clone --recurse-submodules https://github.com/Vincent04190609/edk2.git
cd edk2
```

**Why this configuration matters**:
- `core.autocrlf false` prevents automatic CRLF conversion that causes build issues when moving code between Windows development and Ubuntu testing environments
- `--recurse-submodules` ensures all EDK2 dependencies are cloned in one step
- This strategy must be followed by all developers to maintain consistent line endings across environments

**Verification**: After cloning, verify submodules are initialized with `git submodule status`

### Git Clone Information

For **git clone information** and repository setup details, store in the Paperclip memory system:

- **Type**: `reference` memory
- **Location**: Agent memory files (e.g., `C:\Users\HP\.paperclip\instances\default\companies\{companyId}\agents\{agentId}\memory\`)
- **Format**: Create a dedicated reference memory file like `reference_repo_setup.md` with:
  ```markdown
  ---
  name: Repository Setup
  description: Git clone commands and repository initialization for BIOS development
  type: reference
  ---
  
  # Repository Setup
  
  ## EDK2 Repository
  - Clone command: `git clone https://github.com/tianocore/edk2.git`
  - Submodules: `git submodule update --init --recursive`
  
  ## Project Repository
  
    - refer to **Primary Knowledge Repository**
  
  **Why**: Standardized setup ensures consistent development environments across team members and agent sessions.
  
  **How to apply**: Reference this when onboarding new developers or setting up fresh environments.
  ```

- **Index**: Add an entry to `MEMORY.md`:
  ```markdown
  - [Repository Setup](reference_repo_setup.md) — Git clone commands for EDK2 and project repos
  ```

### When to Update

- Add new clone commands when repositories are added
- Update submodule instructions when dependencies change
- Document any authentication or access requirements

## Code Review and PR Collaboration Protocol

Established in FWB-171 to prevent coordination gaps between code review, approval, and merge.

### Protocol Overview

When code review findings require fixes, CTO/reviewers must follow collaborative paths instead of independent "review→delegate→approve" cycles.

### Required Process

**Path A - Collaborative Fix** (for straightforward code quality issues):
1. Post review findings as a comment on the PR/issue
2. Tag the engineer directly (e.g., @BIOS-Engineer)
3. Engage in real-time discussion on the issue thread
4. Work together to resolve findings
5. Only after collaborative alignment, approve the PR
6. Coordinate with DevOps Engineer for merge strategy and timing

**Path B - Board Escalation** (for significant issues):
1. Document findings clearly
2. Escalate to CEO/board for approval before creating fix tasks
3. Wait for board sign-off on approach
4. Then proceed with implementation

### Key Requirements

- ✅ Use GitHub's formal approval mechanism (not just approval comments)
- ✅ Tag relevant engineers and coordinate in real-time
- ✅ Discuss merge strategy with DevOps before merging
- ✅ Make collaboration visible through issue/PR threads
- ❌ No silent "review→delegate→approve" without engineer engagement
- ❌ No manual merges without following protocol steps

### Enforcement

- Protocol documented in CTO agent instructions
- CEO feedback memory tracks protocol adherence
- Board retains override authority for urgent situations

### Context

- **Origin**: FWB-171 (coordination gap on PR#11)
- **Applied**: FWB-172 (ensures proper PR approval/merge flow)
- **Purpose**: Prevent PRs from being merged without proper collaboration and coordination
