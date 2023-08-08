class Node:

    def __init__(self, node_id, author_email):
        self.id = node_id
        self.label = author_email

    def __repr__(self):
        return f"Node ({self.id}): {self.label}"


class Graph:

    def __init__(self, nodes, edges):
        self.nodes = nodes
        self.edges = edges
        self.node_count = 0
        self.node_labels = {}

    def add_node(self, label):
        """ creates a node with given label """

        node_id = self.node_count + 1
        self.nodes.append(Node(node_id, label))

        # link label with node object
        self.node_labels[label] = self.nodes[-1]
        self.node_count += 1

    def add_edge(self, label_one, label_two):
        """ creates an edge between nodes with given labels """

        if label_one not in self.node_labels or label_two not in self.node_labels:
            print("Error: trying to create edges with non existing labels")
            return

        node_one = self.node_labels[label_one]
        node_two = self.node_labels[label_two]

        # fix this!!!
        if (node_one, node_two) not in self.edges and (node_two, node_one) not in self.edges:
            self.edges.append((node_one, node_two))


    def export(self):
        """ exports graph into a text file """

        # print data about nodes (label and id)

        # print data about edges

        pass
