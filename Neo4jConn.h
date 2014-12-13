#ifndef _dO_Ob_NEO4JCONN_H_
#define _dO_Ob_NEO4JCONN_H_

#include <cstring>
#include <iostream>	
#include <curl/curl.h>	
#include <json/json.h>
#include <stdlib.h>	// realloc, malloc

class Neo4jConn
{
private:
	struct MemoryStruct {
	  char *memory;
	  size_t size;
	};

	Json::Value JSON_DATA;

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
public:
	
	void NewTransaction ()
	{
		JSON_DATA["statements"] = Json::Value(Json::arrayValue);
	}

	void AddTransactionStmt (std::string str_stmt)
	{
		Json::Value json_stmt;
		json_stmt["statement"] = str_stmt.c_str();
		JSON_DATA["statements"].append(json_stmt);
	}

	static std::string Post (Json::Value data, std::string url)
	{
		CURL *curl_handle;
		CURLcode res;
		struct curl_slist *headers = NULL;
	 
		struct MemoryStruct chunk;

		chunk.memory = (char *)malloc(1);  	/* will be grown as needed by the realloc above */ 
		chunk.size = 0;    			/* no data at this point */ 

		headers = curl_slist_append(headers, "Accept: application/json; charset=UTF-8");
		headers = curl_slist_append(headers, "Content-Type: application/json");
	
		std::string data_str = data.toStyledString();	

		curl_handle = curl_easy_init();
		if(curl_handle) {
			curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());			// url set
			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);			// send POST message
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data_str.c_str());//data);//stmts.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data_str.size());	// Will i need CURLOPT_POSTFIELDSIZE_LARGE?
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);	// write response to memory
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);		// write response memory is &chunk
		

			curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

			res = curl_easy_perform(curl_handle);
			if(res != CURLE_OK)
			{
				std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
			}
			else
			{
				std::cout << "Received " << chunk.size << " bytes:\n" << chunk.memory << std::endl;
			}
			curl_easy_cleanup(curl_handle);	
		}
		if(chunk.memory) free(chunk.memory);
		curl_global_cleanup();
		return "";
	}

	static std::string PostTransactionCommit (Json::Value data)
	{
		return Post (data, "http://localhost:7474/db/data/transaction/commit");
	}

	std::string PostTransactionCommit ()
	{
		return Post (JSON_DATA, "http://localhost:7474/db/data/transaction/commit");
	}
};

int main(void)
{
	Neo4jConn Connection;
	Connection.NewTransaction();
	Connection.AddTransactionStmt("MATCH (a) RETURN id(a)");
	Connection.PostTransactionCommit();
}

#endif
