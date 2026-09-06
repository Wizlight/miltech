Import("env")

import subprocess

project_dir = env.subst("$PROJECT_DIR")

git_hash = subprocess.check_output(
    ["git", "rev-parse", "--short", "HEAD"],
    cwd=project_dir,
    text=True
).strip()

git_status = subprocess.check_output(
    ["git", "status", "--porcelain"],
    cwd=project_dir,
    text=True
).strip()

is_dirty = len(git_status) > 0

env.Append(
    CPPDEFINES=[
        ("BUILD_HASH", '\\"{}\\"'.format(git_hash)),
        ("BUILD_DIRTY", 1 if is_dirty else 0)
    ]
)