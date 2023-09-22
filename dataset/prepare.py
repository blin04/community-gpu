"""
Program used for preparing dataset into format that our system expects
"""

source = open("com-amazon.ungraph.txt", 'r')

# samo napraviti da ide o 1 do N....

num_of_nodes = -1
for line in source:
    if line[0] == '#':
        continue

    line = line.split()

    first_node = int(line[0])
    second_node = int(line[1])

    num_of_nodes = max(num_of_nodes, first_node)
    num_of_nodes = max(num_of_nodes, second_node)


source.close()

output = open("nodes", 'w')

for i in range(1, num_of_nodes + 1):
    output.write(str(i) + " X\n")
