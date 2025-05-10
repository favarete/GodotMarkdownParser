extends Control

@onready var richtext_label = $RichTextLabel

const md_example = """
## Text Formatting
You can make text **bold**, _italic_, ~strikethrough~, or `inline code`.  
You can also highlight Some ==very important information== you may have.  

# H1 Heading
## H2 Heading
### H3 Heading
#### H4 Heading
##### H5 Heading

## Lists
### Unordered List
- Item 1
- Item 2
	- Subitem 2.1
	- Subitem 2.2
		- Subitem 2.2.1
			- Subitem 2.2.1.1
				- Subitem 2.2.1.1.1
					- Subitem 2.2.1.1.1.1
- Item 3
- Item 4

### Ordered List
1.  First item
3.  Second item
9.  Third item

### Task List
- [ ] Task 1 (not done)
- [x] Task 2 (done)

## Blockquotes
> This is a blockquote. 
It can span multiple lines 
and can contain **other** _Markdown_ ~elements~.

## Code Blocks
```
function helloWorld() {
  console.log("Hello, world!");
}
```

## Links

[Random Wikipedia Article](https://en.wikipedia.org/wiki/Special:Random)
(Must be handled with the "meta_clicked" signal to have an effect. Check Godot's Documentation)

## Images
### Scaled Images
![](happy_pig.webp) =200
### Custom Dimension Images
![](happy_pig.webp) =150x150
> Image by Sara Miedema / Getty Images

## Tables
### Simple Table
| Header A | Header B |
| -------- | -------- |
| Item 1   | Text 1   |
| Item 2   | Text 2   | 

### Aligned Table
| Left Align  | Center Align | Right Align   |
| :---        |    :----:    |          ---: |
| Item 1      | Text 1       | Content 1     |
| Item 2      | Text 2       | Content 2     |

## Thematic Breaks
A paragraph before the thematic break.

---

A paragraph after the thematic break.
"""

func _ready() -> void:
	var md = MarkdownParser.new()
	var result = md.convert_to_bbcode(md_example)
	richtext_label.text = result
