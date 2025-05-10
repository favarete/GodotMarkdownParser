#include "register_types.h"

#include "core/io/resource_importer.h"
#include "godot_commonmark.h"

void initialize_markdown_parser_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<MarkdownParser>();
}

void uninitialize_markdown_parser_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
