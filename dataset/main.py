from git import Repo
from file import File
from graph import Node, Graph
import time

# loading linux kernel repo
repo = Repo("/home/ivan/projects/linux")
assert not repo.bare

# list of all files in repo
files = {}

# how many latest commits to display (used for debugging)
LIMIT = 200000
start_time = time.time()

print("Starting iteration...")

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

            # add new editors of a file
            if changed_file.name in files:
                files[changed_file.name] += changed_file.editors
            else:
                files[changed_file.name] = changed_file.editors


print("Making list of authors...")
# making list of authors that edited files in linux repo
authors = []
for file, file_authors in files.items():
    for author in file_authors:
        authors.append(author)

print("Making the list unique...")
# making list unique (could fix later)
authors = list(set(authors))

print("Making graph...")
# creating graph
graph = Graph([], [])

print("Creating nodes")
# creating nodes
for author in authors:
    graph.add_node(author)

print("Creating edges...")
# creating edges
for file, file_authors in files.items():
    for first_author in file_authors:
        for second_author in file_authors:

            if first_author == second_author:
                continue

            graph.add_edge(first_author, second_author)

# export graph to a text file
graph.export()

print("--- DONE ---")
print("Number of edges: " + str(len(graph.edges)))

end_time = time.time()
print("Time: " + str(end_time - start_time))