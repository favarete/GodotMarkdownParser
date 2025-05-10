extern "C" {
#include <thirdparty/cmark-gfm/extensions/cmark-gfm-core-extensions.h>
#include <thirdparty/cmark-gfm/extensions/table.h>
#include <thirdparty/cmark-gfm/extensions/tasklist.h>
#include <thirdparty/cmark-gfm/src/cmark-gfm-extension_api.h>
#include <thirdparty/cmark-gfm/src/cmark-gfm.h>
}

#include "godot_commonmark.h"

#include <stdio.h>
#include <string.h>
#include <cstring>
#include <functional>
#include <regex>
#include <unordered_map>

String MarkdownParser::bullet = String::utf8("\u25CF");
bool MarkdownParser::table_header = false;
bool MarkdownParser::tasklist_start = true;
String MarkdownParser::list_type = "none";
int MarkdownParser::table_column_count = 0;
int MarkdownParser::table_column_iteration = 0;
Vector<uint8_t> MarkdownParser::table_alignments = Vector<uint8_t>();

void MarkdownParser::add_markdown_extension(cmark_parser *parser, const char *extName) {
	cmark_syntax_extension *ext = cmark_find_syntax_extension(extName);
	if (ext)
		cmark_parser_attach_syntax_extension(parser, ext);
}

bool MarkdownParser::validate_input(const String &p_string) {
	cmark_gfm_core_extensions_ensure_registered();

	if (p_string.is_empty()) {
		ERR_PRINT("Markdown input is empty.");
		return false;
	}

	if (p_string.length() > 1000000) {
		ERR_PRINT("Markdown input is too large.");
		return false;
	}

	return true;
}

String MarkdownParser::convert_to_html(String p_string) {
	if (!validate_input(p_string)) {
		return "[code]Invalid or too large Markdown input.[/code]";
	}

	cmark_gfm_core_extensions_ensure_registered();

	Vector<uint8_t> string_bytes = p_string.to_utf8_buffer();
	const char *markdown_string = (const char *)string_bytes.ptr();
	int options = CMARK_OPT_HARDBREAKS;

	cmark_parser *parser = cmark_parser_new(options);

	add_markdown_extension(parser, "strikethrough");
	add_markdown_extension(parser, "table");
	add_markdown_extension(parser, "tasklist");

	cmark_parser_feed(parser, markdown_string, string_bytes.size() - 1);

	cmark_node *doc = cmark_parser_finish(parser);

	cmark_parser_free(parser);

	char *html_output = cmark_render_html(doc, options, NULL);
	cmark_node_free(doc);

	String result;
	result.parse_utf8(html_output);

	if (html_output) {
		free(html_output);
	}

	return result;
}

String MarkdownParser::convert_to_bbcode(String p_string) {
	if (!validate_input(p_string)) {
		return "[code]Invalid or too large Markdown input.[/code]";
	}

	cmark_gfm_core_extensions_ensure_registered();
	Vector<uint8_t> string_bytes = p_string.to_utf8_buffer();

	int options = CMARK_OPT_HARDBREAKS;

	cmark_parser *parser = cmark_parser_new(options);
	add_markdown_extension(parser, "table");
	add_markdown_extension(parser, "strikethrough");
	add_markdown_extension(parser, "tasklist");

	cmark_parser_feed(parser, (const char *)string_bytes.ptr(), string_bytes.size());
	cmark_node *root = cmark_parser_finish(parser);

	String buffer;
	process_node(root, buffer);

	cmark_node_free(root);
	cmark_parser_free(parser);

	return buffer;
}

String MarkdownParser::process_node(cmark_node *node, String &buffer) {
	if (!node) {
		return "";
	}

	buffer += node_to_bbcode(node);

	cmark_node *child = cmark_node_first_child(node);
	while (child) {
		process_node(child, buffer);
		child = cmark_node_next(child);
	}

	buffer += node_closing_bbcode(node);

	return buffer;
}

bool MarkdownParser::is_task_list(cmark_node *node) {
	cmark_node *child = cmark_node_first_child(node);
	while (child) {
		std::string child_type = cmark_node_get_type_string(child);
		if (String(child_type.c_str()) == "tasklist") {
			return true;
		}
		child = cmark_node_next(child);
	}

	return false;
}

int MarkdownParser::get_list_depth(cmark_node *node) {
	int depth = 0;
	while (node) {
		if (cmark_node_get_type(node) == CMARK_NODE_LIST) {
			depth++;
		}
		node = cmark_node_parent(node);
	}
	return depth;
}

String MarkdownParser::node_to_bbcode(cmark_node *node) {
	if (!node) {
		return "";
	}

	static const std::unordered_map<std::string, std::function<String(cmark_node *)>> bbcode_map = {
		{ "text", [](cmark_node *node) {
			 String text_literal = String::utf8(cmark_node_get_literal(node));
			 std::string std_text_literal = text_literal.utf8().get_data();

			 std::regex pattern("==([^=]+)==");
			 std::smatch matches;
			 if (std::regex_search(std_text_literal, matches, pattern)) {
				 if (matches.size() > 1) {
					 std::string formatted_text = std::regex_replace(
							 std_text_literal,
							 pattern,
							 "[table=1,center][cell bg=#6a4eff][color=white]$1[/color][/cell][/table]");
					 return String(formatted_text.c_str());
				 }
			 }
			 return text_literal;
		 } },
		{ "paragraph", [](cmark_node *node) {
			 if (list_type == "unordered" | list_type == "ordered") {
				 return String("\n");
			 }
			 if (list_type == "task") {
				 return String("");
			 }
			 return String("[p]");
		 } },
		{ "strong", [](cmark_node *node) {
			 return String("[b]");
		 } },
		{ "softbreak", [](cmark_node *node) {
			 return String("\n");
		 } },
		{ "linebreak", [](cmark_node *node) {
			 return String("\n");
		 } },
		{ "emph", [](cmark_node *node) {
			 return String("[i]");
		 } },
		{ "strikethrough", [](cmark_node *node) {
			 return String("[s]");
		 } },
		{ "link", [](cmark_node *node) {
			 return String("[url=") + String::utf8(cmark_node_get_url(node)) + "]";
		 } },
		{ "image", [](cmark_node *node) {
			 cmark_node *alt_node = cmark_node_first_child(node);
			 String alt_text = alt_node ? String::utf8(cmark_node_get_literal(alt_node)) : "";
			 String image_url = String::utf8(cmark_node_get_url(node));

			 String dimensions;
			 String image_path = image_url;

			 cmark_node *next_sibling = cmark_node_next(node);
			 if (next_sibling && cmark_node_get_type(next_sibling) == CMARK_NODE_TEXT) {
				 String sibling_text = String::utf8(cmark_node_get_literal(next_sibling)).strip_edges();
				 if (sibling_text.begins_with("=")) {
					 dimensions = sibling_text.substr(1);
				 }

				 cmark_node_unlink(next_sibling);
				 cmark_node_free(next_sibling);
			 }

			 if (!dimensions.is_empty()) {
				 return String("[p][img=" + dimensions + "]" + image_path + "[/img][/p]\n");
			 }

			 return String("[p][img]" + image_path + "[/img][/p]\n");
		 } },
		{ "code", [](cmark_node *node) {
			 return String("[table=1,center][cell bg=\"#e8e8e8\"][code]" + String::utf8(cmark_node_get_literal(node)));
		 } },
		{ "code_block", [](cmark_node *node) {
			 return String("[p]\n[table=5,center][cell][/cell][cell][/cell][cell][/cell][cell][/cell][cell bg=\"#e8e8e8\" padding=15,15,15,15][code]" + String::utf8(cmark_node_get_literal(node)));
		 } },
		{ "block_quote", [](cmark_node *node) {
			 return String("[p][table=7,center][cell][/cell][cell][/cell][cell][/cell][cell][/cell][cell bg=\"#adb5bd\"][/cell][cell bg=\"#f6f6f6\"][/cell][cell bg=\"#f6f6f6\"][color=\"#495057\"][i]");
		 } },
		{ "list", [](cmark_node *node) {
			 if (MarkdownParser::is_task_list(node)) {
				 list_type = "task";
				 return String("[p]");
			 }

			 cmark_list_type _list_type = cmark_node_get_list_type(node);
			 switch (_list_type) {
				 case CMARK_BULLET_LIST:
					 list_type = "unordered";
					 return String("[ul bullet=" + bullet + "]");
				 case CMARK_ORDERED_LIST:
					 list_type = "ordered";
					 return String("[ol]");
				 default:
					 list_type = "none";
					 return String("[p]");
			 }
		 } },
		{ "tasklist", [](cmark_node *node) {
			 String format_tasklist_item = "";
			 if (tasklist_start) {
				 tasklist_start = false;
			 } else {
				 format_tasklist_item = "\n";
			 }
			 if (cmark_gfm_extensions_tasklist_is_checked(node) == 1) {
				 return String(format_tasklist_item + String::utf8("\u2705"));
			 }
			 return String(format_tasklist_item + String::utf8("\U0001F532"));
		 } },
		{ "thematic_break", [](cmark_node *node) {
			 return String("\n\n[p][center][color=#dbdbdb]" + String::utf8("\u23AF").repeat(24) + "[/color][/center][/p]\n\n");
		 } },
		{ "html_inline", [](cmark_node *node) {
			 return String("[code](Inline HTML is not supported)[/code]");
		 } },
		{ "html_block", [](cmark_node *node) {
			 return String("[code](HTML block is not supported)[/code]");
		 } },
		{ "table", [](cmark_node *node) {
			 String result = "[p][table=";
			 table_column_count = cmark_gfm_extensions_get_table_columns(node);
			 const uint8_t *alignments = cmark_gfm_extensions_get_table_alignments(node);

			 if (!alignments) {
				 print_line("Warning: alignments is nullptr");
				 return String("[code]Error: Unable to get table alignments[/code]");
			 }
			 table_alignments.clear();
			 for (int i = 0; i < table_column_count; i++) {
				 table_alignments.push_back(alignments[i]);
			 }

			 result += String::num(table_column_count) + ", center]\n";
			 return String(result);
		 } },
		{ "table_header", [](cmark_node *node) {
			 table_header = true;
			 return String("");
		 } },
		{ "table_row", [](cmark_node *node) {
			 table_column_iteration = 0;
			 return String("");
		 } },
		{ "table_cell", [](cmark_node *node) {
			 if (table_header) {
				 return String("[cell padding=5,5,5,5 bg=#e1e4e5 border=#dcdfe1][center][b]" + String::utf8("\u0020").repeat(10));
			 }

			 String alignment_tag;
			 if (table_column_iteration < table_alignments.size()) {
				 uint8_t cell_alignment = table_alignments[table_column_iteration];
				 switch (cell_alignment) {
					 case 108: // 'l'
						 alignment_tag = "[left]";
						 break;
					 case 99: // 'c'
						 alignment_tag = "[center]";
						 break;
					 case 114: // 'r'
						 alignment_tag = "[right]";
						 break;
					 default:
						 alignment_tag = "";
				 }
			 } else {
				 print_line("Warning: table_column_iteration out of bounds");
				 alignment_tag = "";
			 };
			 return String("[cell padding=5,5,5,5 bg=#f3f6f6,#ffffff border=#dcdfe1]" + alignment_tag);
		 } },
		{ "heading", [](cmark_node *node) {
			 int level = cmark_node_get_heading_level(node);
			 if (level == 1)
				 return "[p]" + String::utf8("\u0020") + "[/p]" + String("[p][font_size=45][b]");
			 if (level == 2)
				 return "[p]" + String::utf8("\u0020") + "[/p]" + String("[p][font_size=40][b]");
			 if (level == 3)
				 return "[p]" + String::utf8("\u0020") + "[/p]" + String("[p][font_size=35][b]");
			 if (level == 4)
				 return "[p]" + String::utf8("\u0020") + "[/p]" + String("[p][font_size=30][b]");
			 if (level == 5)
				 return String("[p][font_size=25][b]");
			 return "[p]" + String::utf8("\u0020") + "[/p]" + String("[p][font_size=20][b]");
		 } },
		{ "item", [](cmark_node *node) {
			 if (list_type == "unordered") {
				 int depth = MarkdownParser::get_list_depth(node);

				 switch (depth) {
					 case 1:
						 bullet = String::utf8("\u25E6");
						 break;
					 case 2:
						 bullet = String::utf8("\u2022");
						 break;
					 default:
						 bullet = String::utf8("\u2023");
						 break;
				 }
			 }

			 return String("");
		 } },

	};

	std::string node_type = cmark_node_get_type_string(node);

	// print_line("----->");
	// print_line("Node IN");
	// print_line(String(node_type.c_str()));
	// print_line("<-----");

	auto it = bbcode_map.find(node_type);
	if (it != bbcode_map.end()) {
		return it->second(node);
	}
	return "";
}

String MarkdownParser::node_closing_bbcode(cmark_node *node) {
	if (!node) {
		return "";
	}

	static const std::unordered_map<std::string, std::function<String(cmark_node *)>> bbcode_closing_map = {
		{ "strong", [](cmark_node *node) {
			 return String("[/b]");
		 } },
		{ "emph", [](cmark_node *node) {
			 return String("[/i]");
		 } },
		{ "strikethrough", [](cmark_node *node) {
			 return String("[/s]");
		 } },
		{ "link", [](cmark_node *node) {
			 return String("[/url]");
		 } },
		{ "code", [](cmark_node *node) {
			 return String("[/code][/cell][/table]");
		 } },
		{ "code_block", [](cmark_node *node) {
			 return String("[/code][/cell][/table][/p]\n");
		 } },
		{ "block_quote", [](cmark_node *node) {
			 return String("[/i][/color][/cell][/table][/p]\n");
		 } },
		{ "list", [](cmark_node *node) {
			 if (list_type == "unordered") {
				 return String("[/ul]");
			 }
			 if (list_type == "ordered") {
				 return String("[/ol]");
			 }
			 return String("[/p]");
		 } },
		{ "table", [](cmark_node *node) {
			 return String("[/table][/p]");
		 } },
		{ "table_header", [](cmark_node *node) {
			 table_header = false;
			 return String("");
		 } },
		{ "table_row", [](cmark_node *node) {
			 return String("\n");
		 } },
		{ "table_cell", [](cmark_node *node) {
			 if (table_header) {
				 return String(String::utf8("\u0020").repeat(10) + "[/b][/center][/cell]");
			 }
			 String alignment_tag;
			 if (table_column_iteration < table_alignments.size()) {
				 uint8_t cell_alignment = table_alignments[table_column_iteration];
				 switch (cell_alignment) {
					 case 108: // 'l'
						 alignment_tag = "[/left]";
						 break;
					 case 99: // 'c'
						 alignment_tag = "[/center]";
						 break;
					 case 114: // 'r'
						 alignment_tag = "[/right]";
						 break;
					 default:
						 alignment_tag = "";
				 }
			 } else {
				 print_line("Warning: table_column_iteration out of bounds");
				 alignment_tag = "";
			 }
             table_column_iteration++;
			 return String(alignment_tag + "[/cell]");
		 } },
		{ "heading", [](cmark_node *node) {
			 return String("[/b][/font_size][/p]") + "[p]" + String::utf8("\u0020") + "[/p]";
		 } },
		{ "paragraph", [](cmark_node *node) {
			 if (list_type == "unordered" | list_type == "task" | list_type == "ordered") {
				 return String("");
			 }
			 return String("[/p]");
		 } },
	};

	std::string node_type = cmark_node_get_type_string(node);

	// print_line("----->");
	// print_line("Node OUT");
	// print_line(String(node_type.c_str()));
	// print_line("<-----");

	auto it = bbcode_closing_map.find(node_type);
	if (it != bbcode_closing_map.end()) {
		return it->second(node);
	}
	return "";
}
