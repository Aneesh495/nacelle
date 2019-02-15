#!/usr/bin/env python3
"""
Linter script to ensure no em dashes (or en dashes used as em dashes) exist
in any source code, comments, documentation, or commit messages.
"""

import os
import sys

FORBIDDEN_CHARS = [
    ("\u2014", "em dash (Unicode U+2014)"),
    ("\u2013", "en dash (Unicode U+2013)"),
]

def check_file(filepath):
    errors = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            for line_no, line in enumerate(f, 1):
                for char, desc in FORBIDDEN_CHARS:
                    if char in line:
                        errors.append(f"{filepath}:{line_no}: Found {desc}: {line.strip()}")
                # Also check for spaced double hyphen ( -- ) which is often used as an em dash substitute
                if " -- " in line:
                    errors.append(f"{filepath}:{line_no}: Found ' -- ' used as em dash substitute: {line.strip()}")
    except Exception as e:
        errors.append(f"{filepath}: Error reading file: {e}")
    return errors

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    all_errors = []
    ignored_dirs = {".git", ".system_generated", "build", "dist", "__pycache__"}
    
    for dirpath, dirnames, filenames in os.walk(root_dir):
        dirnames[:] = [d for d in dirnames if d not in ignored_dirs]
        for fname in filenames:
            if fname == "check_no_em_dashes.py":
                continue
            ext = os.path.splitext(fname)[1]
            if ext in {".fst", ".fsti", ".c", ".h", ".py", ".md", ".txt", ".sh", ".json", ".yaml", ".yml", ".mk", ""}:
                filepath = os.path.join(dirpath, fname)
                errors = check_file(filepath)
                all_errors.extend(errors)
                
    if all_errors:
        print("LINTER ERROR: Found forbidden punctuation:")
        for err in all_errors:
            print("  " + err)
        sys.exit(1)
    else:
        print("PASS: No em dashes or improper dashes detected.")
        sys.exit(0)

if __name__ == "__main__":
    main()
