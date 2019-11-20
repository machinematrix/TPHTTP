#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "HttpServer.h"
#include "Vector.h"

VectorHandle files;

void logger(const char *msg)
{
	printf("%s\n", msg);
}

void loadFile(const char *fileName, void **outBuffer, long *outSize)
{
	FILE *file = fopen(fileName, "rb");

	*outSize = 0;
	*outBuffer = NULL;

	if (file)
	{
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		rewind(file);
		void *buffer = malloc((size_t)size);

		if (buffer)
		{
			size_t bytesRead = fread(buffer, 1, (size_t)size, file);
			if (bytesRead == size) {
				*outSize = size;
				*outBuffer = buffer;
			}
			else //either a reading error occurred or the end of file was reached while reading
				free(buffer);
		}

		fclose(file);
	}
}

VectorHandle loadDirectoryFileNames(const char *directory, const char *extension)
{
	VectorHandle files = Vector_Create(sizeof(char*));
	DIR *currDir = opendir(directory);

	if (files && currDir)
	{
		struct dirent *entry;

		while (entry = readdir(currDir))
		{
			if (!extension || (strlen(extension) < strlen(entry->d_name) && !strcmp(entry->d_name + strlen(entry->d_name) - strlen(extension), extension)))
			{
				char *auxName = malloc(strlen(entry->d_name) + 1);

				if (auxName)
				{
					strcpy(auxName, entry->d_name);
					Vector_PushBack(files, &auxName);
				}
			}
		}
	}

	return files;
}

void freeDirectoryFileNames(VectorHandle files)
{
	if (files)
	{
		size_t i, sz;
		for (i = 0, sz = Vector_Size(files); i < sz; ++i)
			free(*(char**)Vector_At(files, i));

		Vector_Destroy(files);
	}
}

void list(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);
	char strBodySize[16];
	const char *bodyBeg = "<a href=\"/", *bodyMid = "\">", *bodyEnd = "</a><br />";
	char *body;
	size_t bodySize = 0, i, sz, bodyBegSize = strlen(bodyBeg), bodyMidSize = strlen(bodyMid), bodyEndSize = strlen(bodyEnd);
	int offset;

	for (i = 0, sz = Vector_Size(files); i < sz; ++i)
	{
		bodySize += bodyBegSize + strlen(*(char**)Vector_At(files, i)) + bodyMidSize + strlen(*(char**)Vector_At(files, i)) + bodyEndSize;
	}

	body = malloc(bodySize);
	sprintf(strBodySize, "%d", bodySize);

	for (i = 0, sz = Vector_Size(files), offset = 0; i < sz; ++i)
	{
		offset += sprintf(body + offset, "%s%s%s%s%s", bodyBeg, *(char**)Vector_At(files, i), bodyMid, *(char**)Vector_At(files, i), bodyEnd);
	}

	HttpServer_SetResponseStatusCode(response, 200);
	HttpServer_SetResponseField(response, HttpResponseField_ContentType, "text/html");
	HttpServer_SetResponseField(response, HttpResponseField_ContentLength, strBodySize);
	HttpServer_SetResponseField(response, HttpResponseField_CacheControl, "no-store");
	HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
	HttpServer_SetResponseBody(response, body, bodySize);
	HttpServer_SendResponse(response);

	free(body);

	HttpServer_DestroyResponse(response);
}

void image(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);

	if (response)
	{
		void *fileBuffer;
		long fileSize;
		loadFile(HttpServer_GetRequestUri(req) + 1, &fileBuffer, &fileSize);

		if (fileBuffer && fileSize)
		{
			char strSize[32];
			sprintf(strSize, "%d", fileSize);
			HttpServer_SetResponseStatusCode(response, 200);
			HttpServer_SetResponseField(response, HttpResponseField_ContentLength, strSize);
			HttpServer_SetResponseField(response, HttpResponseField_ContentType, "image/jpeg");
			HttpServer_SetResponseField(response, HttpResponseField_CacheControl, "no-store");
			HttpServer_SetResponseBody(response, fileBuffer, (size_t)fileSize);

			free(fileBuffer);
		}
		else {
			HttpServer_SetResponseStatusCode(response, 404);
		}

		HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
		HttpServer_SendResponse(response);
		HttpServer_DestroyResponse(response);
	}
}

void redirectToList(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);

	if (response)
	{
		HttpServer_SetResponseStatusCode(response, 303);
		HttpServer_SetResponseField(response, HttpResponseField_ContentType, "text/html");
		HttpServer_SetResponseField(response, HttpResponseField_CacheControl, "no-store");
		HttpServer_SetResponseField(response, HttpResponseField_Location, "/list");
		HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
		HttpServer_SendResponse(response);
	}

	HttpServer_DestroyResponse(response);
}

//poner un archivo favicon.ico en la carpeta del programa para que se muestre en las pesta?as del navegador
void favicon(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);

	if (response)
	{
		void *fileBuffer;
		long fileSize;
		char strBodySize[16];
		loadFile("./favicon.ico", &fileBuffer, &fileSize);

		if (fileBuffer && fileSize)
		{
			sprintf(strBodySize, "%d", fileSize);
			HttpServer_SetResponseStatusCode(response, 200);
			HttpServer_SetResponseField(response, HttpResponseField_ContentType, "image/x-icon");
			HttpServer_SetResponseField(response, HttpResponseField_ContentLength, strBodySize);
			HttpServer_SetResponseField(response, HttpResponseField_CacheControl, "no-store");
			HttpServer_SetResponseBody(response, fileBuffer, fileSize);
			free(fileBuffer);
		}
		else {
			HttpServer_SetResponseStatusCode(response, 404);
		}

		HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
		HttpServer_SendResponse(response);
	}

	HttpServer_DestroyResponse(response);
}

int main(int argc, char **argv)
{
	//system("pwd");
	ServerError error;
	HttpServerHandle server = HttpServer_Create(80, &error);

	if (server)
	{
		files = loadDirectoryFileNames(".", ".jpg");

		if (files)
		{
			size_t i, sz;
			for (i = 0, sz = Vector_Size(files); i < sz; ++i)
			{
				char *resourcePath = malloc(strlen(*(char**)Vector_At(files, i)) + 2);
				if (resourcePath)
				{
					sprintf(resourcePath, "/%s", *(char**)Vector_At(files, i));
					HttpServer_SetEndpointCallback(server, resourcePath, image);
					free(resourcePath);
				}
			}

			HttpServer_SetEndpointCallback(server, "/list", list);
			HttpServer_SetEndpointCallback(server, "/", redirectToList);
			HttpServer_SetEndpointCallback(server, "/favicon.ico", favicon);
			HttpServer_SetLoggerCallback(server, logger);

			HttpServer_Start(server);

			if (HttpServer_GetStatus(server) == ServerStatus_Running)
			{
				char continuar[50] = { 0 };

				while (strcmp(continuar, "exit"))
				{
					scanf("%s", &continuar);
				}
			}
			else
				printf("%s\n", HttpServer_GetServerError(HttpServer_GetErrorCode(server)));
		}

		freeDirectoryFileNames(files);
		HttpServer_Destroy(server);
	}
	else {
		printf("Error: %s\n", HttpServer_GetServerError(error));
		system("read -n1 -r -p \"Press any key to continue...\" key");
	}

    return 0;
}