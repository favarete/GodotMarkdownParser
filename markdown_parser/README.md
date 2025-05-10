## CommonMark Port for Godot
A simple port for parsing markdown into Godot's BBCode, with some custom solutions for conversions with no equivalent.

## Supported
### Basic Syntax
- Heading
- Bold
- Italic
- Blockquote
- Ordered List
- Unordered List
- Inline Code
- Horizontal Rule
- Link
- Image*

*I removed the "alt text" feature from the images because Godot's BBCode show only one word for some reason

### Extended Syntax
- Table*
- Code Block**
- Strikethrough
- Task List
- Highlight

*Without Alignment for now, but I plan on doing it
**Without Syntax Highlighting, brcause I don't see a way of doing it with Godot's BBCode

## Not Supported
- Footnote
- Heading ID
- Definition List
- Emoji
- Subscript
- Superscript

All these features are supported by the parser, but I still need to find a way to implement at the Godot's side, some of then are not possible, probably, due to limitations at Godot's BBCode implementation


## Future
I am thinking about using [RmlUi](https://github.com/mikke89/RmlUi) for rendering markdown using the markdown-to-html parser (already implemented in this module). This would make it possible to write themes and full support everything (it would even make it possible to write UI with html/css inside Godot). But I need the time and energy to investigate that.

