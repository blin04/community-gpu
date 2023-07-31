class File:

    def __init__(self, name):
        self.name = name
        self.editors = []

    def __str__(self):
        return "PATH: " + self.name

    def add_editor(self, editor):
        # adds a new editor to this file
        self.editors.append(editor)
