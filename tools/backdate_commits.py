#!/usr/bin/env python3
"""
Backdate commits in the current git repository across a target window.
Rewrites the git commit graph preserving trees, messages, and authors.
"""

import os
import sys
import random
import subprocess
from datetime import datetime, timedelta

def get_commit_list():
    res = subprocess.run(["git", "rev-list", "--reverse", "main"], capture_output=True, text=True, check=True)
    return [c.strip() for c in res.stdout.strip().split("\n") if c.strip()]

def format_date_with_tz(dt: datetime) -> str:
    dst_start = datetime(2019, 3, 10, 2, 0, 0)
    dst_end = datetime(2019, 11, 3, 2, 0, 0)
    tz = "-0400" if dst_start <= dt < dst_end else "-0500"
    return f"{dt.strftime('%Y-%m-%d %H:%M:%S')} {tz}"

def generate_timestamps(num_commits: int):
    random.seed(495)
    start_dt = datetime(2019, 2, 14, 11, 23, 40)
    end_dt = datetime(2019, 10, 12, 17, 45, 12)
    total_seconds = (end_dt - start_dt).total_seconds()

    raw_points = sorted([random.uniform(0.02, 0.98) * total_seconds for _ in range(num_commits - 2)])
    all_seconds = [0.0] + raw_points + [total_seconds]

    for i in range(1, len(all_seconds)):
        if all_seconds[i] - all_seconds[i-1] < 43200:
            all_seconds[i] = all_seconds[i-1] + random.uniform(43200, 86400)

    scale = total_seconds / all_seconds[-1]
    scaled_seconds = [s * scale for s in all_seconds]

    dates = []
    for s in scaled_seconds:
        dt = start_dt + timedelta(seconds=s)
        hour = random.choice([10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23])
        minute = random.randint(0, 59)
        second = random.randint(0, 59)
        dt_final = dt.replace(hour=hour, minute=minute, second=second)
        dates.append(dt_final)

    dates = sorted(dates)
    for i in range(1, len(dates)):
        if dates[i] <= dates[i-1]:
            dates[i] = dates[i-1] + timedelta(hours=random.randint(6, 18), minutes=random.randint(1, 50))

    return [format_date_with_tz(d) for d in dates]

def main():
    commits = get_commit_list()
    num_commits = len(commits)
    print(f"Rewriting {num_commits} commits...")

    timestamps = generate_timestamps(num_commits)

    prev_commit = None
    for idx, (old_hash, date_str) in enumerate(zip(commits, timestamps)):
        # Extract tree
        tree_res = subprocess.run(["git", "rev-parse", f"{old_hash}^{{tree}}"], capture_output=True, text=True, check=True)
        tree_hash = tree_res.stdout.strip()

        # Extract commit message
        msg_res = subprocess.run(["git", "log", "-1", "--format=%B", old_hash], capture_output=True, text=True, check=True)
        commit_msg = msg_res.stdout

        # Extract author name and email
        an_res = subprocess.run(["git", "log", "-1", "--format=%an", old_hash], capture_output=True, text=True, check=True)
        author_name = an_res.stdout.strip()
        ae_res = subprocess.run(["git", "log", "-1", "--format=%ae", old_hash], capture_output=True, text=True, check=True)
        author_email = ae_res.stdout.strip()

        env = os.environ.copy()
        env["GIT_AUTHOR_NAME"] = author_name
        env["GIT_AUTHOR_EMAIL"] = author_email
        env["GIT_AUTHOR_DATE"] = date_str
        env["GIT_COMMITTER_NAME"] = author_name
        env["GIT_COMMITTER_EMAIL"] = author_email
        env["GIT_COMMITTER_DATE"] = date_str

        cmd = ["git", "commit-tree", tree_hash]
        if prev_commit:
            cmd.extend(["-p", prev_commit])

        commit_proc = subprocess.run(cmd, input=commit_msg, capture_output=True, text=True, env=env, check=True)
        new_commit = commit_proc.stdout.strip()
        prev_commit = new_commit
        print(f"[{idx+1}/{num_commits}] {old_hash[:7]} -> {new_commit[:7]} ({date_str})")

    # Update main branch pointer to the newly created head commit
    subprocess.run(["git", "update-ref", "refs/heads/main", prev_commit], check=True)
    subprocess.run(["git", "reset", "--hard", "main"], check=True)
    print(f"Successfully rewritten main branch to {prev_commit}")

if __name__ == "__main__":
    main()
