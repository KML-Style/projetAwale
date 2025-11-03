typedef struct player
{
    int id;
    int type; //Le joueur 1 ou 2
    char username[BUF_SIZE];
    char bio[100];
    unsigned int score; 
    struct player* friendList;

}player;


void setUsername(char* user, player *p);
void setType(int type, player *p);
void setBio(char bio[100], player *p );
void updateScore(int score, player *p);
void addFriend(player *friend, player *p);
