from git import Repo
from file import File

# loading linux kernel repo
repo = Repo("/home/ivan/Documents/Petnica/Project2023/kernelRepo/linux")
# repo = Repo("/home/ivan/Documents/School/Programiranje/Maturski")
assert not repo.bare

# list of all files in repo
files = {}

# how many latest commits to display
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
            # compared to particular parent commit
            changed_file = None
            for file_diff in diff:
                changed_file = File(file_diff.a_path)
                changed_file.add_editor(commit.author.email)

            if changed_file is None:
                continue

            if changed_file.name in files:
                files[changed_file.name] += changed_file.editors
            else:
                files[changed_file.name] = changed_file.editors


# print out authors of found files
for file, authors in files.items():
    print("==== Authors of file " + file + " are ====")

    for author in authors:
        print(author)