#ifndef GODOT_COMMONMARK_H
#define GODOT_COMMONMARK_H

#include "core/io/resource_saver.h"
#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>


class MarkdownParser : public Resource {
	GDCLASS(MarkdownParser, Resource);

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("convert_to_bbcode", "markdown_string"), &MarkdownParser::convert_to_bbcode);
		ClassDB::bind_method(D_METHOD("convert_to_html", "markdown_string"), &MarkdownParser::convert_to_html);
	}

public:
	String convert_to_bbcode(const String p_string);
	String convert_to_html(const String p_string);

	MarkdownParser() {};
    ~MarkdownParser() {};

private:
	static String bullet;
	static String list_type;
	static bool table_header;
	static bool tasklist_start;
	static int table_column_count;
	static int table_column_iteration;
	static Vector<uint8_t> table_alignments;

    String process_node(cmark_node *node, String &buffer);
    String node_to_bbcode(cmark_node *node);
    String node_closing_bbcode(cmark_node *node);

	bool validate_input(const String &p_string);
	void add_markdown_extension(cmark_parser *parser, const char *extName);

	static bool is_task_list(cmark_node *node);
	static int get_list_depth(cmark_node *node);
};

#endif // GODOT_COMMONMARK_H
