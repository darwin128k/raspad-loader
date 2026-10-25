#!/usr/bin/env python3
"""Generate release notes from Git history; never edits commits or tags."""

import argparse
from collections import OrderedDict
import html
from pathlib import Path
import re
import subprocess
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[1]
CATEGORIES = OrderedDict([
    ('feat', 'Features'), ('fix', 'Fixes'), ('perf', 'Performance'),
    ('refactor', 'Code changes'), ('build', 'Build and packaging'),
    ('ci', 'Automation'), ('docs', 'Documentation'), ('test', 'Tests'),
    ('other', 'Other changes'),
])


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True, encoding='utf-8', stderr=subprocess.PIPE).strip()


def text(value):
    value = html.escape(value, quote=False)
    return re.sub(r'([\\`*_[\]<>])', r'\\\1', value)


def classify(subject):
    match = re.match(r'^(\w+)(?:\([^)]*\))?(!)?:\s*(.+)$', subject)
    if match:
        kind = match[1].lower()
        return kind if kind in CATEGORIES else 'other', match[3], bool(match[2])
    return 'other', subject, False


def history(tag):
    # Resolve to a commit, including annotated tags; only ancestor tags are considered.
    commit = git('rev-parse', '--verify', f'{tag}^{{commit}}')
    try:
        previous = git('describe', '--tags', '--abbrev=0', '--match', 'v*', f'{commit}^')
    except subprocess.CalledProcessError:
        previous = None
    revision = f'{previous}..{commit}' if previous else commit
    records = []
    for line in git('log', '--reverse', '--format=%H%x09%s', revision).splitlines():
        sha, subject = line.split('\t', 1)
        kind, title, breaking = classify(subject)
        records.append(dict(sha=sha, title=title, kind=kind, breaking=breaking))
    return previous, records


def render(tag, repository, previous, records):
    base = f'https://github.com/{repository}'
    release = f'{base}/releases/tag/{quote(tag, safe="")}'
    compare = (f'{base}/compare/{quote(previous, safe="")}...{quote(tag, safe="")}'
               if previous else f'{base}/commits/{quote(tag, safe="")}')
    lines = [f'Raspad Loader **{text(tag)}**', '', '## Highlights', '']
    # Prefer user-facing changes; use commit subjects without inventing a summary.
    priority = {'feat': 0, 'fix': 1, 'perf': 2, 'build': 3, 'ci': 4}
    ranked = sorted(records, key=lambda r: (-int(r['breaking']), priority.get(r['kind'], 5)))
    seen = set()
    for record in ranked:
        if record['title'] in seen:
            continue
        seen.add(record['title'])
        prefix = '**Breaking change:** ' if record['breaking'] else ''
        lines.append(f"- {prefix}{text(record['title'])}.")
        if len(seen) == 5:
            break
    if not records:
        lines.append('- No code changes since the previous tag.')
    count = len(records)
    scope = f'since **{text(previous)}**' if previous else 'in the initial release history'
    lines += ['', f'{count} commits {scope}. [Compare changes]({compare}).', '',
              '## Downloads', '',
              'Choose your game, Windows/Linux x86, and auth variant (Vellum or RevEmu 2013)',
              f'from the [release assets]({release}). SHA-256 checksums are included.',
              'Archives contain the game wrapper and steamclient; use the HL executable and game files from your installation.',
              '', '<details>', '<summary>Full changelog</summary>', '']
    for kind, heading in CATEGORIES.items():
        group = [r for r in records if r['kind'] == kind]
        if not group:
            continue
        lines += [f'### {heading}', '']
        for record in group:
            prefix = '**Breaking change:** ' if record['breaking'] else ''
            lines.append(f"- {prefix}{text(record['title'])} ([{record['sha'][:7]}]({base}/commit/{record['sha']}))")
        lines.append('')
    if not records:
        lines += ['No commits in this range.', '']
    lines += [f'[Full diff]({compare})', '', '</details>', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--repository', required=True, help='owner/repository')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._/-]*', args.tag):
        parser.error('invalid tag/ref')
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', args.repository):
        parser.error('invalid repository')
    previous, records = history(args.tag)
    args.output.write_text(render(args.tag, args.repository, previous, records), encoding='utf-8')
    print(f'Release notes: {len(records)} commits; previous tag: {previous or "none"}')


if __name__ == '__main__':
    main()
