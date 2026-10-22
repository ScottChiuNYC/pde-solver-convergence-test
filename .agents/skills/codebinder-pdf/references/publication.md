# CodeBinder publication

## Pipeline

```text
Markdown/tool validation
-> CodeBinder conversion
-> Sphinx / LaTeX build
-> structural and rendered-text PDF validation
-> GitHub Actions artifact
-> optional Google Drive upload from main
```

The workflow does not execute project notebooks or application/research code merely to create the PDF.

## Triggers

`.github/workflows/publish-codebinder-pdf.yml` runs on:

- pull requests targeting `main`;
- pushes to `main`;
- manual dispatch.

Pull-request runs build and validate the PDF but do not publish to Google Drive.

## Google Drive

Drive publication is optional and controlled by four repository secrets:

```text
GOOGLE_DRIVE_CLIENT_ID
GOOGLE_DRIVE_CLIENT_SECRET
GOOGLE_DRIVE_REFRESH_TOKEN
GOOGLE_DRIVE_FOLDER_ID
```

Configuration states:

- 0/4 present: Drive publication is disabled; PDF build/artifact still succeeds.
- 4/4 present: Drive publication is enabled on successful `main` publication runs.
- 1-3/4 present: fail closed with a configuration error.

Only the upload step receives Google credentials. Do not write credential material into the repository or PDF artifact.

## Review surface

By default, tracked semantic and agent-facing repository content remains visible to CodeBinder. Use `.codebinderignore` for generated, transient, secret-bearing, or deliberately non-reviewable material, not simply because a file is primarily read by an AI agent.
