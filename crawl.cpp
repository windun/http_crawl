/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <iostream>
#include <stdio.h>
#include <cstdio>
#include <cstring>		// memset ()
#include <string>
#include <map>
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
	const char * input;
	std::multimap<std::string, std::unordered_set<std::string>*> Tag_Map;
	std::stack<std::string> Tag_Stack;
	int c = 0;

	void process ()
	{
		bool is_tag = false;
		std::string name_tag = "";

		switch (input[c])
		{
		case ' ':
			break;
		case '<':
			// If the next characters are a series of 
			// letters, we will call it a tag
			is_tag = true;
			int c_ = c + 1;

			// Open tag
			if (input[c_] != '/')
			{
				while (input[c_] != ' ' && input[c_] != '\0')
				{
					// Not a tag if there is anything 
					// but letters
					if (input[c_] < 65 || input[c_] > 122)
					{
						is_tag = false;
						break;
					}
					name_tag += input[c_];
					c_++;
				}
				// If it is a tag, put it on the stack 
				if (is_tag)
				{
					Tag_Stack.push(name_tag);
					Tag_Map.insert(
							std::pair<std::string, std::unordered_set<std::string>*>(name_tag, new std::unordered_set<std::string> ())
					);
				}
				break;
			}
			// Close tag
			else
			{
				while (input[c_] != ' ')
				{
					if (input[c_] < 65 || input[c_] > 122)
					{
						is_tag = false;
						break;
					}	
					name_tag = input[c_] + name_tag;
					c--;
				}
				if (is_tag)
				{
					if (Tag_Stack.top() == name_tag)
					{
						Tag_Stack.pop();
					}
					else
					{
						fprintf(stdout, "Improper tag nesting.\n");
					}
				}
			}
		}
	}

public:
	Parser (const char *input_, struct UrlData *data)
	{
			input = input_;
	}
	int get_urls ()
	{
		while (input[c] != '\0')
		{
			process();
			c++;
		}
	}
	void print ()
	{
		for (std::multimap<std::string, std::unordered_set<std::string>*>::iterator it = Tag_Map.begin(); it != Tag_Map.end(); it++)
		{
			fprintf(stdout, "tag: %s\n", it->first.c_str());
		}
	}
};

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *data)
{
	//int written = fwrite(ptr, size, nmemb, ((struct UrlData *)data)->file);
	//get_urls((char *)ptr, (struct UrlData *)data);
	Parser Parser_((char *)ptr, (struct UrlData *)data);
	Parser_.get_urls();
	Parser_.print();
	return 0;
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
