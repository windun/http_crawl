/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <iostream>
#include <stdio.h>
#include <cstdio>
#include <cstring>		// memset ()
#include <string>
#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <curl/curl.h>		// http gathering

// link recognition (vs. file)
#include <sys/types.h>
#include <regex.h> 

#define REGEX_PATTERN "[a-zA-Z0-9./]*(.bmp|.gif|.jpg|.pdf|.png)"
#define DATA_DIR "data/"
#define INDENT_STEP 5
#define COLUMN_WIDTH 40
#define MAX_BUF_SIZE 1000000
#define MSG_BUF_SIZE 4096
#define MAX_CURL_DEPTH 1


// unordered_map
// target url
//	unordered_set
//	<- origin url
//	<- origin url
// target url //	unordered_set
//	<- origin url
//	...
//std::unordered_multimap<std::string, std::string> URLS;
std::unordered_map<std::string, std::unordered_set<std::string>*> URLS;

struct UrlData
{
	std::string source;
	FILE * file;
	char buffer [MAX_BUF_SIZE];
	int buf_size = 0;
	int level;
};
void print_indent (int indent)
{
	for (int i = 0; i < indent; i++)
	{
		fprintf(stdout, " ");
	}
}
void print_column (const char * str, int width)
{
	for (int i = 0; i < width && str[i] != '\0'; i++)
	{
		fprintf(stdout, "%c", str[i]);
	}
	fprintf(stdout, "\n");
}

void add_url (std::string source, std::string target)
{
	std::unordered_set<std::string> *target_urls = URLS.at(source);
	target_urls->insert(target);
}

bool is_link (char * str, int level)
{
	int indent = level * INDENT_STEP;
	std::string url;
	regex_t regex;
	int reti;
	char buf [MSG_BUF_SIZE];

	// match image files, is_link() should return false for
	// thee matches, so we will negate the result
	reti = regcomp(&regex, REGEX_PATTERN, REG_EXTENDED);
	if (reti){fprintf(stderr, "Could not compile regex\n"); exit(1);}

	reti = regexec(&regex, str, 0, NULL, 0);
	if(!reti)
	{
		url = "[+] "; url += str;
		print_indent(level * INDENT_STEP);print_column(url.c_str(), COLUMN_WIDTH);
		return true;
	}
	else if (reti == REG_NOMATCH)
	{	
		url = "[ ] "; url += str;
		print_indent(level * INDENT_STEP);print_column(url.c_str(), COLUMN_WIDTH);
		return false;
	}
	else
	{
		regerror(reti, &regex, buf, sizeof(buf));
		fprintf(stderr, "Regex match failed: %s\n", buf);
		exit(1);
	}
}
std::string toString (char *c_str)
{
	std::string str = "";
	// Convert only the symbols, letters, and numbers
	// to strings. I had a hard time with an ASCII 13
	// (carriage regurn) being copied into the string.
	for (int i = 0; c_str[i] >= 33 && c_str[i] <= 126; i++)
	{
		str += c_str[i];	
	} 
	return str;
}
std::string toString (const char *c_str)
{
	std::string str = "";
	for (int i = 0; c_str[i] != '\0'; i++)
	{
		str += c_str[i];	
	} 
	return str;
}




class Parser
{
private:
	struct Attribute
	{
		std::string name;
		std::string value;
	};
	struct Tag
	{
		int id;
		int nest_id;
		std::string type;
		std::list<Attribute*>* attributes;
		std::string content;
	};

	int PRINT_WIDTH = 10;
	bool debug = false;
	const char * input;
	std::list<Tag> Tag_List;
	std::stack<Tag> Tag_Stack;
	int c = 0;
	bool in_tag = false;
	int global_id = 1;

	void skip_spaces ()
	{
		// Skip space (32), tab (9), newline
		if(debug) print_section("skip_spaces(), before loop");
		while (input[c] == 32 || input[c] == 9 || input[c] == '\n')
		{
			if(debug) print_section("skip_spaces(), loop");
			c++;
		}
	}
	std::string open_tag ()
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
			if(debug) print_section("open_tag() sees \"" + char_string(c_) + "\"");
			tag_type += input[c_++];
		}
		c = c_;
		return tag_type;
	}

	void get_attribute (Tag &tag)
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
					if (input[c] == ' ') break;
				}
				if(debug) print_section("get_attribute() value += \"" + char_string(c) + "\"");
				attrib->value += input[c++];
			}
			c++;
		}
		if(debug) print_section("get_attribute() stored \"" + attrib->name + "\" = \"" + attrib->value + "\"");
		tag.attributes->push_back(attrib);
	}

	// We are in a tag, now we need to fill in the  Tag struct.
	// We will stop filling in when we get to the end of the
	// end of the tag .../> or ...> or EOF
	void process_tag (std::string tag_type)
	{
		Tag tag;
		tag.type = tag_type;
		tag.id = global_id++;
		tag.nest_id = Tag_Stack.size() == 0 ? 0 : Tag_Stack.top().id;
		tag.attributes = new std::list<Attribute*> ();

		if(debug) print_section("process_tag() begin");
		Tag_List.push_back(tag);
		Tag_Stack.push(tag);

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
			if(debug) print_section("process_tag() closed " + Tag_Stack.top().type);
			Tag_Stack.pop();
		}
	}

	std::string close_tag ()
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
		Tag tag = Tag_Stack.top();
		if ((tag_name == "script" || tag_name == "Script" || tag_name == "SCRIPT") && (tag.type == "script" || tag.type == "Script" || tag.type == "SCRIPT"))
		{
			return "";
		}
		c++;
		if (tag.type == tag_name)
		{
			Tag_Stack.pop();
		}
		else
		{
			print_section ("error: close tag \"" + tag_name + "\" isn't matched.");
		}
		return tag_name;
	}

	void process_content ()
	{
		Tag tag = Tag_Stack.top();
		if(debug) print_section("process_content() begin");
		while (input[c] != '\0')
		{
			if (input[c] == '<') // && !(tag.type == "script" || tag.type == "Script" || tag.type == "SCRIPT"))
			{
				if(debug) print_section("process_content() break");
				break;
			}
			//if(debug) print_section("process_content() for " + Tag_Stack.top().type);
			tag.content += input[c++];
		}
		if(debug) print_section("process_content() for " + tag.type + " found: " + tag.content);
	}

	void process_comment ()
	{

		if(debug) print_section ("process_comment() begin");
		if (Tag_Stack.top().type != "!--")
		{
			if(debug) print_section ("process_comment() Error: top is \"" + Tag_Stack.top().type + "\"");
			fprintf(stderr, "Error, comment tag is not Tag_Stack.top().\n");
			exit(1);
		}
		Tag tag = Tag_Stack.top();
		while (input[c] != '\0')
		{
			if (input[c] == '-' && input[c] == '-' && input[c] =='>')
			{
				c = c + 3;
				if(debug) print_section("process_comment() found comment: " + tag.content);
				Tag_Stack.pop();
			}
			tag.content += input[c++];
		}
	}
	void process ()
	{
		std::string tag_type;
		while (input[c] != '\0')
		{
			skip_spaces(); if(debug) print_section("process() skip_spaces");
			tag_type = open_tag (); if(debug) print_section("process() open_tag found: \"" + tag_type + "\"");
			if(tag_type != "")
			{
				process_tag (tag_type);
				process_content ();
			}
			else
			{
				tag_type = close_tag ();
				if (tag_type == "")
				{
					process_content();
				}
			}
		}
	}

	void print_section (std::string comment)
	{
		for (int i = c; input[i] != '\0' && i < (c + PRINT_WIDTH); i++)
		{
			std::cout << char_string(i);
		}
		std::cout << "      " << comment << std::endl;
	}

	std::string char_string (int i)
	{
		std::string str;
		if (input[i] < 32)
		{
			str = "|";
			str += (int)input[i];
			str += "|";
		}
		else
		{
			str = std::string(1, input[i]);
		}
		return str;
	}

public:
	Parser (const char *input_)
	{
			input = input_;
	}
	void set_debug (bool show_debug)
	{
		debug = show_debug;
	}
	int get_urls ()
	{
		if(debug) std::cout << "get_urls\n";
		process ();
	}
	void print ()
	{
		for (std::list<Tag>::iterator it = Tag_List.begin(); it != Tag_List.end(); it++)
		{
			std::cout << it->id << "," << it->nest_id << "  " << it->type << std::endl;
			for (std::list<Attribute*>::iterator it_attr = it->attributes->begin(); it_attr != it->attributes->end(); it_attr++)
			{
				std::cout << "     " << (*it_attr)->name << " = " << (*it_attr)->value << std::endl;
			}
			std::cout << "     content = \"" << it->content << "\"" << std::endl;
		}
	}
	std::list<std::string>* get_attribute_values (std::string attrib)
	{

	}
};

static size_t write_header (void *ptr, size_t size, size_t nmemb, void *data)
{
	int written = fwrite(ptr, size, nmemb, ((struct UrlData *)data)->file);
	return written;
}

// called during curl_easy_perform ()
// ptr = data from internet
// size = #packets ?
// nmemb = amount data per packet
// data = my data
static size_t write_body (void *ptr, size_t size, size_t nmemb, void *data)
{
	UrlData *url_data = (struct UrlData *)data;
	char *mem_buf = url_data->buffer;
	int written = fwrite(ptr, size, nmemb, url_data->file);

	int mem_size = size * nmemb;
	memcpy(&(mem_buf[url_data->buf_size]), (char *)ptr, mem_size);
	url_data->buf_size += mem_size;
	mem_buf[url_data->buf_size] = 0;	// null terminate 1 index past end

	return written;
}


int mCurl (const char* source_url, int level)
{
	//Parser Parser_((char *)ptr, (struct UrlData *)data);
	//Parser_.set_debug(true);
	//Parser_.get_urls();
	//Parser_.print();

	struct UrlData header_data;
	struct UrlData body_data;

	CURL *curl_handle;
	CURLcode res;
	std::string headerFilename = DATA_DIR + std::to_string(level) + "_" + std::string(source_url) + "_head.out";
	std::string bodyFilename = DATA_DIR + std::to_string(level) + "_" + std::string(source_url) + "_body.out";
	
	try
	{
		std::unordered_set<std::string> *target_urls = URLS.at(source_url);
	}
	catch (std::exception& e)
	{
		URLS.insert({{source_url, new std::unordered_set<std::string> ()}});
	}

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if(curl_handle)
	{

		/////////////////////////////////////////////
		//	SET UP CURL
		//std::string heading = "[" + level + "]-> " + source_url + "\n";
		std::string heading = "[" + std::to_string(level) + "]-> " + source_url;
		print_indent(level * INDENT_STEP); print_column (heading.c_str(), COLUMN_WIDTH);
		curl_easy_setopt(curl_handle, CURLOPT_URL, source_url);
		//curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);	
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_body);

		/////////////////////////////////////////////
		//	SET UP FILES
		header_data.file = fopen(headerFilename.c_str(), "wb");
		if (header_data.file == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		body_data.file = fopen(bodyFilename.c_str(), "wb");
		if (body_data.file == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		header_data.source = toString(source_url);
		header_data.level = level;
		body_data.source = toString(source_url);
		body_data.level = level;

		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &header_data);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body_data);


 		/////////////////////////////////////////////
		//	PERFORM THE CURL
		res = curl_easy_perform(curl_handle);


		
		if (res != CURLE_OK)
		{
			print_indent(level * INDENT_STEP); fprintf(stdout, "[x]-> %s\n", curl_easy_strerror(res));
			remove(headerFilename.c_str());
			remove(bodyFilename.c_str());
		}
		curl_easy_cleanup(curl_handle);
		
		Parser Parser_((char *)body_data.buffer);
		Parser_.set_debug(true);
		Parser_.get_urls();
		Parser_.print();

		std::string footer = "[+] " + std::to_string(URLS.at(source_url)->size()) + " urls found.";
		print_indent(level * INDENT_STEP); print_column (footer.c_str(), COLUMN_WIDTH);
		int n = 0; 
		std::string previous;
		for (std::unordered_set<std::string>::iterator it=URLS.at(source_url)->begin(); it != URLS.at(source_url)->end(); it++, n++)
		{
			if(level >= MAX_CURL_DEPTH)
			{
				return 0;
			}
			else
			{
				mCurl(it->c_str(), level + 1);
			}
			previous = *it;
		}
	}	
	return 0;

}
int main (int argc, char* argv[])
{
	mCurl(argv[1], 0);	
}
