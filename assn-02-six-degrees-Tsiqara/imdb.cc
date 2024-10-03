using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

struct Node{
  char* file;
  char* key;
  int year;
};

// searched should be Node
int actor_cmpfun(const void* searched, const void* offsetFromStart){
  char* startPointer = ((struct Node*)searched)->file;

  char* key = ((struct Node*)searched)->key;
  char* arrayMember = startPointer + (*(int*)offsetFromStart);
  
  return strcmp(key, arrayMember);
}

int film_cmpfun(const void* film, const void* offsetFromStart){
  char* startPointer = ((struct Node*)film)->file;

  char* key = ((Node*)film)->key;
  char* arrayMember = startPointer + (*(int*)offsetFromStart);
  if(string(key) == string(arrayMember)){
    arrayMember += string(arrayMember).size() + 1;
    if(((Node*)film)->year == 1900 + *(arrayMember)){
      return 0;
    }else if(((Node*)film)->year < 1900 + *(arrayMember)){
      return -1;
    }else{
      return 1;
    }
  }

  return strcmp(key, arrayMember);
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const { 
  void* base = (int*)actorFile + 1;
  int actorNum = *((int*)actorFile);
  int elementSize = sizeof(int);

  Node key;
  key.file = (char*)actorFile;
  key.key = (char*)player.c_str();

  char* start = (char*)bsearch(&key, base, actorNum, elementSize, actor_cmpfun);

  if(start == NULL){
    return false;
  }

// start points at the start of int which indicates offset of player info from the begining of the actorFile
  start = (char*)actorFile + *(int*)start;

  //offset of movieNumber from the start of the player info
  int movieNumberOffset =  player.size() % 2 == 0 ? player.size() + 2 : player.size() + 1;
  short movieNumber = *((short*)start + movieNumberOffset/2);

  // offset to array of movies from the start of the player info
  int* movies;
  if((movieNumberOffset + sizeof(short)) % 4 == 2){
    movies = (int*)start + (movieNumberOffset + sizeof(short) + 2)/4;
  }else{
    movies = (int*)start + (movieNumberOffset + sizeof(short))/4;
  }

  for(int i = 0; i < movieNumber; i ++){
    int curOffset = *movies;
    char* curPointer = (char*)movieFile + curOffset;
    film f;
    f.title = string(curPointer);
    curPointer += f.title.size() + 1;
    f.year = 1900 + (int)*(curPointer);
    films.push_back(f);
    ++ movies;
  }

  return true;
}


bool imdb::getCast(const film& movie, vector<string>& players) const { 
  void* base = (int*)movieFile + 1;
  int numberOfMovies = *((int*)movieFile);
  int elementSize  = sizeof(int);

  Node key;
  key.file = (char*)movieFile;
  key.key = (char*)(movie.title.c_str());
  key.year = movie.year;

  char* start = (char*)bsearch(&key, base, numberOfMovies, elementSize, film_cmpfun);

  if(start == NULL){
    return false;
  }

// start points to start of the int which indicates offset of this movie info from the begining of movieFile
  start = (char*)movieFile + *(int*)start;
  start += movie.title.size() + 1;
/* Pointer start is at the start of byte which indicates year
 if movie c_string size and 1 byte for year number is odd we need +2, else +1, to move to the actorNumber. */
  start = (movie.title.size() + 1 + 1) % 2 == 0 ? start + 1 : start + 2;
  short numberOfActors = *(short*)start;

// move moviePointer to array of actors
  start += sizeof(short);
  if(((movie.title.size() + 2) % 2 == 0 && (movie.title.size() + 2 + sizeof(short)) % 4 != 0)
   || ((movie.title.size() + 2) % 2 != 0 && (movie.title.size() + 2 + 1 + sizeof(short)) % 4 != 0)){ 
    start += 2;
  }
  
  for(int i = 0; i < numberOfActors; i ++){
    int curActor = *(int*)start;
    string name = string((char*)actorFile + curActor);
    players.push_back(name);
    start += sizeof(int);
  }

  return true; 
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
