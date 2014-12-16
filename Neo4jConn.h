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
	int transaction_id = 0;


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

	static void PrintStringChars (Json::Value data)
	{
		const char * cstr = data.toStyledString().c_str();
		for (int i = 0; cstr[i] != '\0'; i++)
	  	{
			std::cout << (int)cstr[i] << "|";
		}
		std::cout << std::endl;
	}
	
	void NewTransaction ()
	{
		JSON_DATA["statements"] = Json::Value(Json::arrayValue);
	}

	void AddTransaction (std::string str_stmt, std::string format)
	{
		Json::Value statement;
		statement["statement"] = str_stmt;
		statement["resultDataContents"] = Json::Value(Json::arrayValue);
		statement["resultDataContents"].append(format);
		JSON_DATA["statements"].append(statement);
	}

	void AddTransaction (Json::Value *json)
	{
		//std::cout << "Neo4jConn sees:" << json->toStyledString() << std::endl;
		JSON_DATA["statements"].append(*json);
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
		std::cout << "Sending " << data_str << std::endl;	

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
				std::cout << "Received " << chunk.size << " bytes:\n";// << chunk.memory << "\n" << std::endl;
				Json::Value json_reply;
				Json::Reader reader;
				bool parse_success = reader.parse(chunk.memory, json_reply);
				std::cout << "Response: \n" << json_reply.toStyledString() << std::endl;
				ProcessResult(json_reply);
				
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

	static void ProcessResult (Json::Value value)
	{
		std::cout << "Response (processed): \n";
		//PrintJsonTree(value, 0);


		// Print errors
		Json::Value errors = value["errors"];
		for (int i = 0; i < errors.size(); i++)
		{
			std::cout << "[!] " << errors[i] << std::endl;
		}

		Json::Value results = value["results"];


		// Print columns
		std::cout << "[c] " << std::endl;
		for (int r = 0; r < results.size(); r++)
		{
		
			Json::Value columns = results["columns"];
			for (int i = 0; i < columns.size(); i++)
			{
				std::cout << columns[i] << " |";
			}
			std::cout << std::endl;

	
			// Print data
			Json::Value data = results["data"];
			for (int i = 0; i < data.size(); i++)
			{
				std::cout << "[" << i << "]" << data.toStyledString();
			}
		}
	}

	// http://stackoverflow.com/questions/4800605/iterating-through-objects-in-jsoncpp
	static void PrintJsonTree (Json::Value root, int depth)
	{ 
		depth++;
		if (root.size() > 0)
		{
			for (Json::ValueIterator it = root.begin(); it != root.end(); it++)
			{
				for (int d = 0; d < depth; d++)
				{
					std::cout << "   ";
				}
				PrintValue(it.key());
				std::cout << std::endl;
				PrintJsonTree(*it, depth);
			}
			return;
		}
		else
		{
			for (int d = 0; d < depth; d++)
			{
				std::cout << "   ";
			}
			PrintValue(root); 
			std::cout << std::endl;
		}
		return;
	}

	// http://stackoverflow.com/questions/4800605/iterating-through-objects-in-jsoncpp
	static void PrintValue (Json::Value val)
	{
	    if( val.isString() ) {
		printf( "string(%s)", val.asString().c_str() ); 
	    } else if( val.isBool() ) {
		printf( "bool(%d)", val.asBool() ); 
	    } else if( val.isInt() ) {
		printf( "int(%d)", val.asInt() ); 
	    } else if( val.isUInt() ) {
		printf( "uint(%u)", val.asUInt() ); 
	    } else if( val.isDouble() ) {
		printf( "double(%f)", val.asDouble() ); 
	    }
	    else 
	    {
		printf( "unknown type=[%d]", val.type() ); 
	    }
	}
};

#endif
