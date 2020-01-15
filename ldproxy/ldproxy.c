#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

static void load_lib(void) __attribute__((constructor));
static void unload_lib(void) __attribute__((destructor));

typedef void *(*CreateInterfaceFn)(const char *pName, int *pReturnCode);

static void *g_client;
static CreateInterfaceFn g_CreateInterface;

static int third_party_count;
static void **third_party_libs;

static void load_lib(void)
{
    printf("LDPROXY LOADING..\n");

    // Step 1. Parse /proc/{}/maps for our path
    char *our_path=0;
    int our_path_len;
    char line[512];
    {
        FILE *procmaps = fopen("/proc/self/maps", "r");
        while (fgets(line, sizeof(line), procmaps))
        {
            int start, end;
            sscanf(line, "%*x-%*x %*4c %*x %*x:%*x %*x %n", &start);
            char *path = line+start-1;
            end = strlen(path);
            //sizeof("ldproxy/client.so\n")==18
            int clen = 18;
            if(end>=clen&&!strcmp(&path[end-clen], "ldproxy/client.so\n")){
                printf("LDPROXY found self, line:\n%s", path);
                //clip off the ldproxy/client.so\n
                path[end-clen]='\0';
                our_path = path;
                our_path_len = end-clen;
                while(*our_path==' '){
                    our_path++;
                    our_path_len--;
                }
                printf("LDPROXY path to bin/:\n%s\n", our_path);
                break;
            }
        }
    }
    assert(our_path);
    // Step 2. Parse third_party_libs txt file
    snprintf(line, sizeof(line), "%s/ldproxy.txt", our_path);
    {
        FILE *ldproxytxt;
        assert(ldproxytxt = fopen(line, "r"));
        third_party_count = 0;
        while (fgets(line, sizeof(line), ldproxytxt))
        {
            //skip indentation
            char *i;
            for(i=line;*i==' ';i++);
            //comments and empty lines
            if(*i=='#' || *i == '\0') continue;
            third_party_count++;
        }
        printf("LDPROXY third_party_count: %d\n", third_party_count);
        third_party_libs = malloc(third_party_count*sizeof(void*));
        rewind(ldproxytxt);
        for (int x=0; fgets(line, sizeof(line), ldproxytxt);)
        {
            //skip indentation
            char *i;
            for(i=line;*i==' ';i++);
            //comments and empty lines
            if(*i=='#' || *i == '\0') continue;
            int ilen = strlen(i);
            if(i[ilen-1]=='\n') i[--ilen]='\0';
            assert(ilen+our_path_len+1<sizeof(line));
            memmove(line+our_path_len, i, ilen+1);
            memcpy(line, our_path, our_path_len);
            i = line+our_path_len;
            printf("LDPROXY loading third_party_lib '%s', with path:\n%s\n", i, line);
            third_party_libs[x] = dlopen(line, RTLD_NOW);
            if(!third_party_libs[x]){
                printf("LDPROXY FAILED! Couldn't open third_party_lib '%s'. DLERROR:\n%s\n", i, dlerror());
                exit(1);
            }
            third_party_libs[x];
            x++;
        }
    }
    // Step 3. Load real client.so
    strcat(our_path, "client.so");
    printf("LDPROXY loading %s\n", our_path);
    g_client = dlopen(our_path, RTLD_NOW);
    if(!g_client){
        printf("LDPROXY FAILED! Couldn't open client.so. DLERROR:\n%s\n", dlerror());
        exit(1);
    }
    assert(g_CreateInterface = dlsym(g_client, "CreateInterface"));
    printf("LDPROXY SUCCESS\n");
}

static void unload_lib(void)
{
    dlclose(g_client);
    for(int x=0; x<third_party_count; x++){
        dlclose(third_party_libs[x]);
    }
    free(third_party_libs);
}

__attribute__((visibility("default")))
void *CreateInterface(const char *pName, int *pReturnCode){
   return g_CreateInterface(pName, pReturnCode);
}
