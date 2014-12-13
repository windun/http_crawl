#include <cstring>
#include <iostream>	
#include <curl/curl.h>	
#include <json/json.h>
#include <stdlib.h>	// realloc, malloc

struct MemoryStruct {
  char *memory;
  size_t size;
};

void PrintStringChars (char * cstr)
{
	for (int i = 0; cstr[i] != '\0'; i++)
  	{
		std::cout << (int)cstr[i] << "|";
	}
	std::cout << std::endl;
}

std::string Trim(char * cstr)
{
	std::string buffer = "";
	int int_value;
	for (int i = 0; cstr[i] != '\0'; i++)
  	{
		int_value = (int)cstr[i];
		if (int_value >= 32)
		{
			buffer += cstr[i];
		}
	}
	return buffer;
}

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

std::string POST (std::string post_msg, std::string url)
{
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *headers = NULL;
 
	struct MemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  	/* will be grown as needed by the realloc above */ 
	chunk.size = 0;    			/* no data at this point */ 

	headers = curl_slist_append(headers, "Accept: application/json; charset=UTF-8");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	char data[]="{\"statements\" : [{\"statement\" : \"CREATE (a) RETURN id(a)\"} ]}";
	std::cout << data << std::endl;
	PrintStringChars(data);
	Json::Value stmt;
	Json::Value stmts; 
	stmt["statement"] = "CREATE (a) RETURN id(a)";
	stmts["statements"] = Json::Value(Json::arrayValue);
	stmts["statements"].append(stmt);
	//const char * stmts_char = Trim((char *)stmts.toStyledString().c_str());
	//std::cout << stmts.c_str() << std::endl;
	//std::cout << stmts_char << std::endl;
	std::string stmt_string = stmts.toStyledString();	
	//PrintStringChars((char *)stmts_char);
	curl_handle = curl_easy_init();
	if(curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());			// url set
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);			// send POST message
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, stmt_string.c_str());//data);//stmts.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, stmt_string.size());	// Will i need CURLOPT_POSTFIELDSIZE_LARGE?
		//curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);	// write response to memory
		//curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);		// write response memory is &chunk
		

		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(curl_handle);
		if(res != CURLE_OK)
		{
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else
		{
			//std::cout << "Received " << chunk.size << " bytes.\n";
		}
		curl_easy_cleanup(curl_handle);	
	}
	if(chunk.memory) free(chunk.memory);
	curl_global_cleanup();
	return "";
}

int main(void)
{
	POST ("POST http://localhost:7474/db/data/transaction/commit", "http://localhost:7474/db/data/transaction/commit");
}
