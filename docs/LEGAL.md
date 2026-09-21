# Copyright and publication notes

PocketShelf is an independent software client, not a book repository. It integrates with Z-Library's unofficial API and PocketBook's native reader. It is not affiliated with or endorsed by either service. Names identify compatible services and devices.

## Authorized use

Download only content that is lawfully made available by its source and that you are entitled to use. Public-domain status depends on jurisdiction; an open license must actually apply to the particular work and edition. Owning another copy, having an account, or finding a working link does not by itself authorize a download. PocketShelf cannot determine whether a particular listing is authorized. Native Cloud upload also requires appropriate rights and compliance with the cloud provider's terms.

The project does not supply books, catalog dumps, book covers, shared accounts, credentials, DRM-removal tools, or instructions for bypassing access controls. It uses the user's own session, respects API errors and download availability, and does not circumvent browser challenges or DRM. Ordinary same-origin cookie redirects are supported for authentication. Service terms and applicable law may impose additional restrictions on use of an unofficial API.

## What is published

The repository contains application source, synthetic tests, build/install scripts, and documentation. Generated binaries, SDK/toolchain downloads, local caches, private test keys, device logs, session files, and downloaded books are excluded. No binary releases are published as part of this initial repository setup.

The only vendored library is **cJSON 1.7.19**, under the MIT license. Its source and header are unmodified, and its original copyright and license notice is retained in [`vendor/cJSON.LICENSE`](../vendor/cJSON.LICENSE). All three vendored files were checked byte-for-byte against [the upstream release](https://github.com/DaveGamble/cJSON/tree/v1.7.19).

The PocketBook SDK, InkView, libcurl and build tools are external dependencies. The build script fetches the official SDK separately; the SDK and its libraries are not redistributed here. Publishing compiled distributions requires a separate check of all applicable dependency redistribution terms. Protocol references in [the research notes](research.md) informed the independent implementation; no KOReader plugin is bundled.

No project-wide license is granted by this document. Third-party components retain their own licenses. Making a repository public is not, by itself, an open-source license for the original project code.

## Limits of this review

These notes document the publication checks, not legal clearance. Source-only distribution and an authorized-use notice do not guarantee that publishing or using this specific integration is lawful. Publisher liability, service terms, copyright, and anti-circumvention rules depend on the facts and relevant jurisdictions. Obtain qualified legal advice if you need that assurance before distributing or promoting the project more broadly.

Relevant primary sources checked on 2026-09-21:

- [GitHub Acceptable Use Policies](https://docs.github.com/en/site-policy/acceptable-use-policies/github-acceptable-use-policies): GitHub prohibits content that infringes third-party proprietary rights.
- [GitHub DMCA Takedown Policy](https://docs.github.com/en/site-policy/content-removal-policies/dmca-takedown-policy): explains copyright notices and circumvention claims; hosting on GitHub is not legal approval.
- [German Copyright Act, section 53](https://www.gesetze-im-internet.de/urhg/__53.html): the private-copy provision excludes obviously unlawfully produced or publicly made available sources, among other limitations. This is an example, not an assumption that German law is the only applicable law.
- [German Copyright Act, section 95a](https://www.gesetze-im-internet.de/urhg/__95a.html): addresses protection of effective technological measures.
