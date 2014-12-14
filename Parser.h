#ifndef _dO_Ob_PARSER_H_
#define _dO_Ob_PARSER_H_

#include <algorithm>		// std::transform()
#include <iostream>
#include <json/json.h>
#include <list>
#include <sstream>		// ostringstream in char_string()
#include <string>


class Parser
{
	#define PARSE_COLUMN_WIDTH 40			// process_content()
private:
	struct Attribute
	{
		std::string name;
		std::string value;
	};
	class Tag
	{
	#define MIN_BUF_SIZE 500000
	private:
		char *content;
		std::list<Attribute*>* attributes;
	public:
		int id;
		int nest_id;
		std::string type;
		
		int content_size = 0;
		int content_limit = MIN_BUF_SIZE;

		Tag()
		{
			nest_id = 0;
			content = new char [content_limit];
			attributes = new std::list<Attribute*> ();
		}
		~Tag()
		{
			//std::cout << "~Tag() begin.";
			for (std::list<Attribute*>::iterator attr_it = attributes->begin(); attr_it != attributes->end(); attr_it++)
			{
				delete *attr_it;			// delete items in attributes list
			}
			delete attributes;		// delete the attributes list

			delete[] content;

		}
		void add_content (char c)
		{
			if (content_size >= content_limit)
			{
				std::cout << "[!] content size surpassed (" << content_size << "/" << content_limit << ")" << std::endl;
				char *tmp;
				char *old = content;
				content_limit += 200;
				tmp = new char [content_limit];
				content = (char *)memcpy(tmp, content, content_size);
				delete old;
				std::cout << "[!] content size increased (" << content_size << "/" << content_limit << ")" << std::endl;
			}
			content[content_size++] = c;
		}
		void edit_content (char c, int index)
		{
			content[index] = c;
		}
	
		std::string get_content()
		{
			return std::string(content, content_size);
		}

		std::string get_content(int length)
		{
			return std::string(content, length);
		}

		void addAttribute (Attribute *attrib)
		{
			attributes->push_back(attrib);
		}

		std::list<Attribute*>* getAttributes ()
		{
			return attributes;
		}
	};

	class JsonBuilder
	{
	private:
		// all appearance of a tag of specific type will
		// be agglomerated into a single node in the following
		// fashion:
		//		tag
		//			attribute1: value1, value2
		//			attribute2: value1
		//			etc.
		std::unordered_map<std::string, Json::Value*> JsonTags;		// html -> Json
	public:															// meta -> Json
		void addTag (std::string tag_type)
		{
			Json::Value *tag_entry = new Json::Value();
			if(JsonTags.count(tag_type) == 0)
			{
				std::cout << "[ ] " << tag_type << std::endl;
				JsonTags[tag_type] = tag_entry;						// JsonTags[head]->tag_entry
				(*tag_entry)["query"] = "CREATE (n:" + tag_type + " { props } ) RETURN n";						//						"type" = "head"
				(*tag_entry)["params"] = Json::Value(Json::objectValue);
				((*tag_entry)["params"])["props"] = Json::Value(Json::objectValue);
			}
		}

		void addAttribute(std::string tag_type, Attribute *attrib)
		{
			Json::Value *tag_entry;
			if(JsonTags.count(tag_type) == 0)
			{
				std::cerr << "[!] JsonTags entry doesn't exist for: " << tag_type << std::endl;
			}
			else
			{
				tag_entry = JsonTags[tag_type];
			}
			std::string attrib_value;
			if(attrib->value == "") 
			{
				attrib_value = "null";
			}
			else
			{
				attrib_value = attrib->value;
			}
			(((*tag_entry)["params"])["props"])[attrib->name].append(attrib_value);
		}

		std::list<Json::Value*>* getJson ()
		{
			std::list<Json::Value*>* json_list;
			for (std::unordered_map<std::string, Json::Value*>::iterator it = JsonTags.begin(); it != JsonTags.end(); it++)
			{
				std::cout << "JsonBuilder packing: \n" << it->second->toStyledString() << std::cout;
				json_list->push_back(it->second);
			}
			std::cout << "JsonBuilder done packing.\n";
			return json_list;
		}
	};

	int PRINT_WIDTH = 10;
	bool debug = false;
	const char * input;
	std::list<Tag*> Tag_List;
	std::list<Tag*> Tag_Stack;
	JsonBuilder JsonBuilder;
	int c = 0;
	bool in_tag = false;
	int global_id = 1;

	bool is_tag(std::string lc_str, std::string str);
	void skip_spaces ();
	std::string open_tag ();
	void get_attribute (Tag *tag);
	void process_tag (std::string tag_type);
	std::string close_tag ();
	void process_content ();
	void process_comment ();
	void print_section (std::string comment);
	std::string char_string (int i);
	bool stack_pop(std::string tag_type);
	bool stack_pop(Tag *tag);

public:



	Parser (const char *input_)
	{
			input = input_;
	}
	~Parser ()
	{
		delete_tag_list(Tag_List);
		Tag_Stack.clear();
		//std::cout << "~Parser() Tag_List(" << Tag_List.size() << ") Tag_Stack(" << Tag_Stack.size() << ")" << std::endl;
	}
	void delete_tag_list (std::list<Tag*> &List)
	{
		if(debug) print_section("delete_tag_list()");
		for (std::list<Tag*>::iterator it=List.begin(); it !=List.end(); it++)
		{
			delete *it;
		}
		List.clear();						// clear the list
	}
	void set_debug (bool show_debug)
	{
		debug = show_debug;
	}
	void process ()
	{
		std::string tag_type;
		if (input == 0 ) return;
		Tag *base = new Tag ();
		base->type = "base";
		base->id = 0;
		base->nest_id = 0;
		Tag_List.push_back(base);
		Tag_Stack.push_back(base);
		while (input[c] != '\0')
		{
			skip_spaces(); if(debug) print_section("process() skip_spaces");
			tag_type = open_tag (); if(debug) print_section("process() open_tag found: \"" + tag_type + "\"");
			if(tag_type != "")
			{
				process_tag (tag_type);
				process_content();
			}
			else
			{
				tag_type = close_tag ();
				if (tag_type == "")
				{
					process_content();
				}
			}
			skip_spaces();
		}
	}
	void print_info (std::string ofile_name)
	{
		std::ofstream ofile;
		ofile.open(ofile_name);

		if(ofile.is_open())
		{
			std::cout << Tag_List.size () << " tags.\n";
			for (std::list<Tag*>::iterator it = Tag_List.begin(); it != Tag_List.end(); it++)
			{
				ofile << (*it)->id << "," << (*it)->nest_id << "  " << (*it)->type << std::endl;
				for (std::list<Attribute*>::iterator it_attr = (*it)->getAttributes()->begin(); it_attr != (*it)->getAttributes()->end(); it_attr++)
				{
					ofile << "     " << (*it_attr)->name << " = " << (*it_attr)->value << std::endl;
				}
				if ((*it)->content_size != 0) ofile << "     content (" << (*it)->content_size << ") = \"" << (*it)->get_content() << "...\"" << std::endl;
			}
			ofile.close();
		}
	}
	void get_attribute_values (std::string attrib, std::list<std::string>* result)
	{
		for (std::list<Tag*>::iterator it = Tag_List.begin(); it != Tag_List.end(); it++)
		{
			for (std::list<Attribute*>::iterator it_attr = (*it)->getAttributes()->begin(); it_attr != (*it)->getAttributes()->end(); it_attr++)
			{
				//std::cout << "check" << std::endl;
				if (is_tag(attrib, (*it_attr)->name))
				{
					result->push_back((*it_attr)->value);
				}
			}
		}
	}
	
	std::list<Json::Value*>* getJson ()
	{
		return JsonBuilder.getJson();
	}
};

/*
 *	is_tag () - used to check for specific tags
 *		lc_str - tag used for comparison - provide is all lower case (e.g. "script")
 *		str - check if str is the same tag as that provided in lc_str
 */
bool Parser::is_tag(std::string lc_str, std::string str)
{
	if (lc_str.size() != str.size()) return false;
	std::string::iterator lc = lc_str.begin();
	std::string::iterator s = str.begin();
	while (lc != lc_str.end() && s != str.end())
	{
		if ((int)*s < 91) // if the letter in str is capital case
		{
			if (*lc != (*s + 32)) return false;	// change to lower case version and compare
		}
		else
		{
			if (*lc != *s) return false;
		}
		lc++; s++;
	}
	return true;
}

void Parser::skip_spaces ()
{
	// Skip space (32), tab (9), newline
	if(debug) print_section("skip_spaces(), before loop");
	while ((input[c] == 32 || input[c] == 9 || input[c] == '\n' || input[c] == 10 || input[c] == 13) && input[c] != 0)
	{
		if(debug) print_section("skip_spaces(), loop");
		c++;
		//std::cout << (int)input[c] << "| (=0?" << (input[c] == 0) << ")";
	}
	if (debug) print_section("skip_spaces() finished.");
	//std::cout << std::endl;
}
std::string Parser::open_tag ()
{
	if(debug) print_section("open_tag() begin");
	std::string tag_type = "";
	std::string value;
	int c_ = c;
	if (input[c_] != '<')
	{
		if(debug) print_section("open_tag() did not see <");
		return "";
	}
	if (input[c_ + 1] == '/')
	{
		if(debug) print_section("open_tag() found a close_tag");
		return "";
	}
	c_++;
	while (input[c_] != '\n' && input[c_] != '\0' && input[c_] != '>' && input[c_] != '/' && input[c_] != ' ')
	{
		if (input[c_] < 33 || (input[c_] > 57 && input[c_] < 65) || input[c_] > 122)
		{
			return "";
		}
		value = input[c_];
		//if(debug) print_section("open_tag() sees \"" + char_string(c_) + "\"");
		tag_type += input[c_++];
	}
	c = c_;
	return tag_type;
}

void Parser::get_attribute (Tag *tag)
{
	Attribute *attrib = new Attribute ();
	if(debug) print_section("get_attribute() begin");
	while (input[c] != ' ' && input[c] != '=' && input[c] != '>')
	{
		attrib->name += input[c++];
	}
	if(debug) print_section("get_attribute() found \"" + attrib->name + "\"");
	skip_spaces ();
	if (input[c] != '=')
	{

	}
	else
	{
		if(debug) print_section("get_attribute() saw =");
		c++;
		skip_spaces ();
//			if (input[c] != '"' && input[c] != '\'')
//			{
//				fprintf(stderr, "Error, incorrectly formed attribute.\n");
//				exit(1);
//			}
		char delimiter = 0; //= input[c];
		if (input[c] == '"' || input[c] == '\'')
		{
			delimiter = input[c];
			if(debug) print_section("get_attribute() saw delimiter " + char_string(c));
			c++;
		}
		while (input[c] != '\0')
		{
			if (delimiter != 0)
			{
				if (input[c] == delimiter) break;
			}
			else
			{
				if (input[c] == ' ' || input[c] == '>')
				{
					c--;	// to put the pointer back on ' ' or '>', c++ after break will advance
					break;
				}
			}
			//if(debug) print_section("get_attribute() value += \"" + char_string(c) + "\"");
			attrib->value += input[c++];
		}
		c++;
	}
	if(debug) print_section("get_attribute() stored \"" + attrib->name + "\" = \"" + attrib->value + "\"");
	//tag->attributes->push_back(attrib);
	tag->addAttribute(attrib);
	JsonBuilder.addAttribute(tag->type, attrib);
}

// We are in a tag, now we need to fill in the  Tag struct.
// We will stop filling in when we get to the end of the
// end of the tag .../> or ...> or EOF
void Parser::process_tag (std::string tag_type)
{
	Tag *tag = new Tag ();
	std::transform(tag_type.begin(), tag_type.end(), tag_type.begin(), ::tolower);
	tag->type = tag_type;
	tag->id = global_id++;
	tag->nest_id = Tag_Stack.back()->id;

	if(debug) print_section("process_tag() begin");
	Tag_List.push_back(tag);
	//if(!is_tag("meta", tag_type) && !is_tag("br", tag_type))
	Tag_Stack.push_back(tag);	// do not put meta on tag stack
	JsonBuilder.addTag(tag_type);

	if(tag_type == "!--")
	{
		if (debug) print_section("process_tag() defers to process_comment()");
		process_comment ();
		return;
	}

	// You have the tag, skip white space
	if(debug) print_section("process_tag() skip_spaces");
	skip_spaces ();

	while (input[c] != '/' && input[c] != '>' && input[c] != '\0')
	{
		get_attribute(tag);
		skip_spaces ();
	}

	// Exit the tag definition?
	if (input[c] == '>')
	{
		c++;
		return;
	}

	// Close the tag
	if (input[c] == '/' && input[c + 1] == '>')
	{
		c = c + 2;
		if(debug) print_section("process_tag() finds closed " + Tag_Stack.back()->type);
		if(debug) print_section("process_tag() calls stack_pop()");
		stack_pop(tag);
	}
}

std::string Parser::close_tag ()
{
	if(debug) print_section("close_tag() begin");
	std::string tag_name;
	if (input[c] != '<' && input[c + 1] != '/') return "";
	if(debug) print_section("close_tag() found ...");

	c = c + 2;
	while (input[c] != '>')
	{
		tag_name += input[c++];
	}
	if(debug) print_section("close_tag() found \"" + tag_name + "\"");

	// Check if this should be within a script or terminate the script
	Tag *top_tag = Tag_Stack.back();
	if (is_tag("script", tag_name) && !is_tag("script", top_tag->type))
	{
		return "";
	}
	c++;
	if(debug) print_section("close_tag() calls stack_pop()");
	stack_pop(tag_name);
	return tag_name;
}

void Parser::process_content ()
{
	Tag *tag = Tag_Stack.back();
	if(debug) print_section("process_content() begin");
	while (input[c] != '\0')
	{
		if (input[c] == '<') // && !(tag.type == "script" || tag.type == "Script" || tag.type == "SCRIPT"))
		{
			if(debug) print_section("process_content() break");
			break;
		}
		if(debug) print_section("process_content() for " + Tag_Stack.back()->type);
		//tag->content[tag->content_size++] = input[c++];
		tag->add_content(input[c++]);
	}
	//tag->content[tag->content_size] = 0;
	tag->edit_content(0, tag->content_size);
	if(debug)
	{
		if (is_tag("script", tag->type))
		{
			print_section("process_content() for " + tag->type + " found: " + tag->get_content(PARSE_COLUMN_WIDTH));
		}
		else
		{
			print_section("process_content() for " + tag->type + " found: " + tag->get_content());
		}
	}
}

void Parser::process_comment ()
{

	if(debug) print_section ("process_comment() begin");
	if (Tag_Stack.back()->type != "!--")
	{
		if(debug) print_section ("process_comment() Error: top is \"" + Tag_Stack.back()->type + "\"");
		fprintf(stderr, "Error, comment tag is not Tag_Stack.top().\n");
		exit(1);
	}
	Tag *tag = Tag_Stack.back();
	while (input[c] != '\0')
	{
		if (input[c] == '-')			// trying to avoid going out of boundary
		{
			if (input[c+1] == '-')		//
			{
				if(input[c+2] == '>')	//
				{
					c = c + 3;
					if(debug) print_section("process_comment() found comment: " + tag->get_content());
					Tag_Stack.pop_back();
					break;
				}
			}
		}
		//tag->content[tag->content_size] += input[c++];
		tag->add_content(input[c++]);
		//tag->content[tag->content_size] = 0;
		tag->edit_content(0, tag->content_size);
	}
}

void Parser::print_section (std::string comment)
{
	for (int i = c; input[i] != '\0' && i < (c + PRINT_WIDTH); i++)
	{
		std::cout << char_string(i);
	}
	std::cout << "      " << comment << std::endl;
}


std::string Parser::char_string (int i)
{
	std::string str;

	if (input[i] < 32)
	{
		str = "|";
		str += static_cast<std::ostringstream*>(&(std::ostringstream() << (int)input[i]))->str();
		str += "|";
	}
	else
	{
		str = std::string(1, input[i]);
	}
	return str;
}

bool Parser::stack_pop(std::string tag_type)
{
	if(debug) print_section("stack_pop() found stack size = " + static_cast<std::ostringstream*>(&(std::ostringstream() << Tag_Stack.size()))->str());
	if (Tag_Stack.size() == 0)
	{
		return false;
	}
	if(tag_type == Tag_Stack.back()->type)
	{
		if(debug) print_section("stack_pop() pop stack top (" + Tag_Stack.back()->type + ")");
		Tag_Stack.pop_back();
	}
	else
	{
		if(debug) print_section("stack_pop() tag not on top of stack");
		for (std::list<Tag*>::iterator it = Tag_Stack.end(); it != Tag_Stack.begin();)
		{
			it--;
			if((*it)->type == tag_type)
			{
				if(debug) print_section ("stack_pop() found the tag in stack");
				while (Tag_Stack.back()->type != tag_type)
				{
					if(debug) print_section ("stack_pop() removed " + Tag_Stack.back()->type);
					Tag_Stack.pop_back();
				}
				if(debug) print_section ("stack_pop() removed " + Tag_Stack.back()->type);
				Tag_Stack.pop_back();
				return true;
			}
		}
	}
	return false;
}

bool Parser::stack_pop(Tag *tag)
{
	if(debug) print_section("stack_pop() found stack size = " + static_cast<std::ostringstream*>(&(std::ostringstream() << Tag_Stack.size()))->str());
	if (Tag_Stack.size() == 0)
	{
		return false;
	}
	if(tag == Tag_Stack.back())
	{
		if(debug) print_section("stack_pop() pop stack top (" + Tag_Stack.back()->type + ")");
		Tag_Stack.pop_back();
		return true;
	}
	else
	{
		if(debug) print_section("stack_pop() " + tag->type + " not on top of stack");
		for (std::list<Tag*>::iterator it = Tag_Stack.end(); it != Tag_Stack.begin();)
		{
			it--;
			if(*it == tag)
			{
				if(debug) print_section ("stack_pop() found the tag in stack");
				while (Tag_Stack.back() != tag)
				{
					if(debug) print_section ("stack_pop() removed " + Tag_Stack.back()->type);
					Tag_Stack.pop_back();
				}
				if(debug) print_section ("stack_pop() removed " + Tag_Stack.back()->type);
				Tag_Stack.pop_back();
				return true;
			}
		}
	}
	return false;
}


#endif
