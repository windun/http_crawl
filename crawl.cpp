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
#define MAX_BUF_SIZE 4096
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
	char buf [MAX_BUF_SIZE];

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
		std::list<Attribute> attributes;
		std::string content;
	};

	bool debug = false;
	const char * input;
	std::list<Tag> Tag_List;
	std::stack<Tag> Tag_Stack;
	int c = 0;
	bool in_tag = false;
	int global_id = 0;

	void skip_spaces ()
	{
		// Skip space (32), tab (9), newline
		while (input[c] == 32 || input[c] == 9 || input[c] == '\n')
		{
			if(debug) std::cout << input[c] << " skip_spaces, loop\n";
			c++;
		}
	}
	std::string open_tag ()
	{
		if(debug) std::cout << input[c] << " open_tag\n";
		std::string tag_type = "";
		int c_ = c;
		if (input[c_++] != '<') return "";
		while (input[c_] != '\n' && input[c_] != '\0' && input[c_] != '>' && input[c_] != '/')
		{
			if (input[c_] < 65 || input[c_] > 122)
			{
				return "";
			}
			tag_type += input[c_++];
			if(debug) std::cout << input[c_] << " open_tag\n";
		}
		return tag_type;
	}

	void get_attribute (Tag &tag)
	{
		Attribute attrib;
		if(debug) std::cout << "get_attribute\n";
		while (input[c] != ' ' && input[c] != '=')
		{
			attrib.name += input[c++];
		}
		skip_spaces ();
		if (input[c] != '\"')
		{
			fprintf(stderr, "Error, incorrectly formed attribute.\n");
			exit(1);
		}
		c++;
		while (input[c] != '\"')
		{
			attrib.value += input[c++];
		}
		tag.attributes.push_back(attrib);
	}

	// We are in a tag, now we need to fill in the  Tag struct.
	// We will stop filling in when we get to the end of the
	// end of the tag .../> or ...> or EOF
	void process_tag (std::string tag_type)
	{
		Tag tag;
		tag.type = tag_type;
		tag.id = global_id++;

		if(debug) std::cout << input[c] << " process_tag\n";
		// While not the end of the tag...
		skip_spaces (); std::cout << input[c] << " process_tag,skip_spaces\n";
		while (input[c] != '\\' && input[c] != '>' && input[c] != '\0')
		{
			get_attribute(tag);
			skip_spaces ();
		}
	}

	std::string close_tag ()
	{
		std::string tag_name;
		if (input[c] != '<' && input[c + 1] != '/') return "";
		c = c + 2;
	}

	void process ()
	{
		std::string tag_type;
		while (input[c] != '\0')
		{
			skip_spaces(); if(debug) std::cout << input[c] << " process, skip_spaces\n";
			tag_type = open_tag (); if(debug) std::cout << input[c] << " process, open_tag: " << tag_type << std::endl;
			if(tag_type != "")
			{
				process_tag (tag_type);
			}
		}
	}

public:
	Parser (const char *input_, struct UrlData *data)
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
			fprintf(stdout, "tag: %s\n", it->type.c_str());
		}
	}
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *data)
{
	int written = fwrite(ptr, size, nmemb, ((struct UrlData *)data)->file);
	//get_urls((char *)ptr, (struct UrlData *)data);
	Parser Parser_((char *)ptr, (struct UrlData *)data);
	Parser_.set_debug(true);
	Parser_.get_urls();
	Parser_.print();
	return written;
}

int mCurl (const char* source_url, int level)
{
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
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

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
