from git import Repo
from file import File

# loading linux kernel repo
repo = Repo("/home/ivan/Documents/Petnica/Project2023/kernelRepo/linux")
# repo = Repo("/home/ivan/Documents/School/Programiranje/Maturski")
assert not repo.bare

# list of all files in repo
files = {}

LIMIT = 1000

# iterate over commits in repo
for commit in repo.iter_commits(reverse=True):
    if LIMIT <= 0:
        break

    LIMIT -= 1

    if len(commit.parents):
        # iterate over parent commits
        for parent_commit in commit.parents:
            diff = parent_commit.diff(commit)

            # iterate over files that were changed in current commit
            temp = None
            for file_diff in diff:
                temp = File(file_diff.a_path)
                temp.add_editor(commit.author.email)

                if temp.name in files:
                    files[temp.name] += 1
                else:
                    files[temp.name] = 1

for file, cnt in files.items():
    print(file + ": "  + str(cnt))