# Code signing policy

Sunveil Audio is preparing an application to the SignPath Foundation program.
SignPath signing has not been approved or activated. Current GitHub Actions
artifacts are unsigned development builds for validation.

## Source and build verification

The Windows VST3 builds are generated from this repository by the
[Windows build-and-test workflow](.github/workflows/windows-build.yml).
Each artifact records its source commit and workflow run. The workflow builds
and tests Sundrive, Afterglow, and Glassgarden and retains their license notices.
It does not sign installers or publish end-user releases.

Before signed releases are enabled, the signing configuration must verify the
source and build origin, enforce product metadata, and require manual release
approval. Installer source and packaging must also be included in that verified
build before any installer is submitted for signing.

## Maintainer and approval

- Source maintainer and reviewer: [cyanappy](https://github.com/cyanappy).
- Release approver: [cyanappy](https://github.com/cyanappy).
- Multi-factor authentication is required on repository and SignPath accounts
  before signing is activated.

Signing requests require a deliberate review and approval of the intended
release. Source or build changes must be reviewed before a release is signed.

## Privacy

The current plugin source processes audio locally. It implements no telemetry,
account activation, or automatic network transfer of audio or personal data.
The DAW or plugin host controls its own storage and network behavior.
GitHub Actions processes the public repository source and generated build
artifacts; it does not receive audio from users running the plugins.

## Attribution after approval

If the application is approved and signing is activated, signed release pages
will credit SignPath.io for free code signing and SignPath Foundation for the
certificate. The certificate will be issued in SignPath Foundation's name.
This pending application is not an endorsement or a safety certification.

The [SignPath Foundation conditions](https://signpath.org/terms) govern any
future use of the program.
