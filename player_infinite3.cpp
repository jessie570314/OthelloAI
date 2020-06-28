#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <queue>
using namespace std;

struct Point {
    int x, y;
    Point(float x, float y) : x(x), y(y) {}
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Point& rhs) const {
        return !operator==(rhs);
    }
    Point operator+(const Point& rhs) const {
        return Point(x + rhs.x, y + rhs.y);
    }
    Point operator-(const Point& rhs) const {
        return Point(x - rhs.x, y - rhs.y);
    }
};
int board_value[8][8] =
{

    {20,-3,11, 8, 8,11,-3,20},
    {-3,-10,-4, 1, 1,-4,-10,-3},
    {11,-4, 2, 2, 2, 2,-4,11},
    { 8, 1, 2,-3,-3, 2, 1, 8},
    { 8, 1, 2,-3,-3, 2, 1, 8},
    {11,-4, 2, 2, 2, 2,-4,11},
    {-3,-10,-4, 1, 1,-4,-10,-3},
    {20,-3,11, 8, 8,11,-3,20},
};

int player;
const int SIZE = 8;
std::array<std::array<int, SIZE>, SIZE> board;
const std::array<Point, 8> directions{{
    Point(-1, -1), Point(-1, 0), Point(-1, 1),
    Point(0, -1), /*{0, 0}, */Point(0, 1),
    Point(1, -1), Point(1, 0), Point(1, 1)
}};

class State{
public:
    enum SPOT_STATE {
           EMPTY = 0,
           BLACK = 1,
           WHITE = 2
       };
    std::array<int, 3> disc_count;
    std::array<std::array<int, SIZE>, SIZE> board;
    int heuristic;
    int mobility[3]={0,0,0};
    int stability[3]={0,0,0};
    int currentMove;
    int prevMove;
    bool done=false;
    int ex,ey;
    int winner=-1;
    void get_stability();
    void flip_discs(Point center,int type);
    void setHeuristic(int type);
    bool is_disc_at(Point p, int disc) const {
        if (!is_spot_on_board(p))return false;
        if (board[p.x][p.y] != disc)return false;
        return true;
    }
    void get_mobility(int play);
    vector<Point> next_valid_spots;
    std::vector<Point> valid_spots;
    void get_next(int play);
    bool is_spot_on_board(Point p) const{
        return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE;
    }
    bool is_spot_valid(Point center,int type) const {
        if (board[center.x][center.y] != EMPTY)
            return false;
        for (Point dir: directions) {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, 3-type))continue;
            p = p + dir;
            while (is_spot_on_board(p) && board[p.x][p.y] != EMPTY) {
                if (is_disc_at(p, type))
                    return true;
                p = p + dir;
            }
        }
        return false;
    }
    State(){}
    State(array<array<int, SIZE>, SIZE> board){
        for(int i=0;i<8;i++){
            for(int j=0;j<8;j++){
                this->board[i][j]=board[i][j];
            }
        }
        //setHeuristic();
    }
    State(const State& s){
        this->heuristic=s.heuristic;
        this->ex=s.ex;
        this->ey=s.ey;
        this->done=s.done;
        this->next_valid_spots=s.next_valid_spots;
        for(int i=0;i<8;i++){
            for(int j=0;j<8;j++){
                this->board[i][j]=s.board[i][j];
            }
        }
    }
    void update(int newx, int newy,int player);
    bool operator<(const State& rhs) const;
};
bool State::operator<(const State& rhs)const{
    return heuristic<rhs.heuristic;
}
void State::setHeuristic(int type){
    if(done &&winner!=type){
        
        heuristic=-10000;
        return;
    }
    else if(done && winner==type){
        heuristic=10000;
        return;
    }
    int w=0;
    int n=0;
    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            if(board[i][j]==type){
                w+=board_value[i][j];
            }
            else if(board[i][j]==3-player){
                n+=board_value[i][j];
            }
        }
    }
    int corner=0,cornerzone=0;
    if(board[0][0]==type)corner++;
    else if(board[0][0]==3-type)corner--;
    else{
        if(board[0][1]==type || board[1][0]==type)cornerzone-=1;
        if(board[1][1]==type)cornerzone-=2;
    }
    if(board[0][7]==type)corner++;
    else if(board[0][7]==3-type)corner--;
    else{
        if(board[0][6]==type || board[1][7]==type)cornerzone-=1;
        if(board[1][6]==type)cornerzone-=2;
    }
    if(board[7][0]==type)corner++;
    else if(board[7][0]==3-type)corner--;
    else{
        if(board[7][1]==type || board[6][0]==type)cornerzone-=1;
        if(board[6][1]==type)cornerzone-=2;
    }
    if(board[7][7]==type)corner++;
    else if(board[7][7]==3-type)corner--;
    else{
        if(board[7][6]==type || board[6][7]==type)cornerzone-=1;
        if(board[6][6]==type)cornerzone-=2;
    }
    get_stability();
    int num=0;
    if(disc_count[type]+disc_count[3-type]>40)num=disc_count[type]-disc_count[3-type];
    int parity=2;
    parity =(64-(disc_count[1]+disc_count[2]))<18 ? ((64-(disc_count[1]+disc_count[2]))%2==0?-1:1) : 0;
    if(type==2)parity=-parity;
    if(parity==-1){
        if(mobility[3-type]==0)parity=100;
    }
    corner+=cornerzone;
    if(mobility[3-type]==0)mobility[type];
    /*heuristic=17*corner+3*(mobility[type]-mobility[3-type])+7*(w-n)+6*num+7*(stability[type]-stability[3-type]);
    if(disc_count[type]+disc_count[3-type]>53)heuristic=6*num+2*(mobility[type]-mobility[3-type]);*/
   /* if(disc_count[type]+disc_count[3-type]<20)heuristic=8*(w-n)+3*(mobility[type]-mobility[3-type])+7*(stability[type]-stability[3-type]);
    else if(disc_count[type]+disc_count[3-type]<40)heuristic=8*(w-n)+3*(mobility[type]-mobility[3-type])+num+7*(stability[type]-stability[3-type]);
    else heuristic=(w-n)+3*(mobility[type]-mobility[3-type])+5*num+7*(stability[type]-stability[3-type]);*///best of now
    if(disc_count[type]+disc_count[3-type]<20)heuristic=8*(w-n)+3*(mobility[type]-mobility[3-type])+7*(stability[type]-stability[3-type])-num+3*corner;
    else if(disc_count[type]+disc_count[3-type]<40)heuristic=25*corner+3*(mobility[type]-mobility[3-type])+1.5*num+11*(stability[type]-stability[3-type]);
    else if(disc_count[type]+disc_count[3-type]<55)heuristic=25*corner+3*(mobility[type]-mobility[3-type])+7*num+11*(stability[type]-stability[3-type]);
    else heuristic=num;
};
void State::get_stability(){
    if(board[0][0]!=0){
        int i,j;
        for(i=0;i<8;i++){
            if(board[0][i]==board[0][0])stability[board[0][0]]++;
            else break;
        }
        for(j=0;j<8;j++){
            if(board[j][0]==board[0][0])stability[board[0][0]]++;
            else break;
        }
    /*
        for(int k=1;k<=j;k++){
            for(int l=1;l<=i;l++){
                if(board[k][l]==board[0][0])stability[board[0][0]]++;
                else{
                    break;;
                }
            }
        }*/
    }
    if(board[0][7]!=0){
        int i,j;
        for(i=7;i>=0;i--){
            if(board[0][i]==board[0][7])stability[board[0][7]]++;
            else break;
        }
        for(j=0;j<8;j++){
            if(board[j][0]==board[0][7])stability[board[0][7]]++;
            else break;
        }
      /*  for(int k=1;k<=j;k++){
            for(int l=6;l>=i;l--){
                if(board[k][l]==board[0][7])stability[board[0][7]]++;
                else{
                    break;
                }
            }
        }*/
    }
    if(board[7][7]!=0){
        int i,j;
        for(i=7;i>=0;i--){
            if(board[0][i]==board[7][7])stability[board[7][7]]++;
            else break;
        }
        for(j=7;j>=0;j--){
            if(board[j][0]==board[7][7])stability[board[7][7]]++;
            else break;
        }
       /* for(int k=6;k>=j;k++){
            for(int l=6;l>=i;l--){
                if(board[k][l]==board[7][7])stability[board[7][7]]++;
                else{
                    break;
                }
            }
        }*/
    }
    if(board[7][0]!=0){
        int i,j;
        for(i=0;i<=7;i--){
            if(board[0][i]==board[7][0])stability[board[7][0]]++;
            else break;
        }
        for(j=7;j>=0;j--){
            if(board[j][0]==board[7][0])stability[board[7][0]]++;
            else break;
        }
       /* for(int k=6;k>=j;k++){
            for(int l=1;l<=i;l--){
                if(board[k][l]==board[7][0])stability[board[7][0]]++;
                else{
                    break;
                }
            }
        }*/
    }
}
void State::get_next(int type)  {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            Point p = Point(i, j);
            if (board[i][j] != EMPTY)
                continue;
            if (is_spot_valid(p,type)){
               // State a=State(board);
                valid_spots.push_back(p);
                mobility[type]++;
               // a.update(p.x, p.y, type);
               // next_valid_spots.push_back(a);
               // printf("valid%d %d\n",valid_spots.back().x,valid_spots.back().y);
            }
            
        }
    }
    
}
void State::get_mobility(int type)  {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            Point p = Point(i, j);
            if (board[i][j] != EMPTY)
                continue;
            if (is_spot_valid(p,type)){
                mobility[type]++;
            }
            
        }
    }
    
}
int get_disc(Point p,array<array<int, SIZE>, SIZE> nboard)  {
       return nboard[p.x][p.y];
}

void State::flip_discs(Point center,int type) {
    for (Point dir: directions) {
        Point p = center + dir;
        if (!is_disc_at(p, 3-type))
               continue;
        std::vector<Point> discs({p});
        p = p + dir;
        while (is_spot_on_board(p) && get_disc(p,board) != EMPTY) {
            if (is_disc_at(p, type)) {
                for (Point s: discs) {
                    board[s.x][s.y]=type;
                }
                disc_count[type] += discs.size();
                disc_count[3-type] -= discs.size();
                break;
            }
            discs.push_back(p);
            p = p + dir;
           }
       }
   }

void State::update(int newx, int newy,int player){
    this->board[newx][newy]=player;
    this->ex=newx;
    this->ey=newy;
    this->disc_count[player]++;
    this->disc_count[EMPTY]--;
    this->flip_discs(Point(ex,ey),player);
    this->get_next(player);
    this->get_mobility(3-player);
    setHeuristic(player);
}

priority_queue<State> pq;
void read_board(std::ifstream& fin) {
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
        }
    }
}
void read_valid_spots(std::ifstream& fin) {
    int n_valid_spots;
    fin >> n_valid_spots;
    int x, y;
    for (int i = 0; i < n_valid_spots; i++) {
        fin >> x >> y;
    }
}


Point search(int depth,State a,int type,int alpha,int beta,std::ofstream& fout){
    if(depth<0){
        return Point(a.ex,a.ey);
    }
    a.get_next(type);
    if(type==player){
        int max=-10000;
        Point ans(0,0);
        for(unsigned long i=0;i<a.valid_spots.size();i++){
            State cur=a;
            cur.update(a.valid_spots[i].x, a.valid_spots[i].y, type);
            Point c=search(depth-1,cur,3-type,alpha,beta,fout);
            State temp=cur;
            temp.update(c.x, c.y, type);
            if (temp.heuristic > max) {
                max= temp.heuristic;
                alpha = max;
                ans=Point(a.valid_spots[i].x, a.valid_spots[i].y);
                if(depth==5){
                    fout << ans.x << " " << ans.y << std::endl;
                    fout.flush();
                }
               // printf("%d %d\n",c.x,c.y);
            }
            if (alpha >= beta) {
                //printf("%d %d\n",c.x,c.y);
                break;
            }
        }
        //printf("aaaaa%d %d\n",ans.x,ans.y);
        return ans;
    }
    else{
        int min=10000;
        Point ans(0,0);
        for(unsigned long i=0;i<a.valid_spots.size();i++){
            State cur=a;
            cur.update(a.valid_spots[i].x, a.valid_spots[i].y, type);
            Point c=search(depth-1,cur,3-type,alpha,beta,fout);
            State temp=cur;
            temp.update(c.x, c.y, type);
            if (temp.heuristic < min) {
                min= temp.heuristic;
                beta = min;
                ans=Point(a.valid_spots[i].x, a.valid_spots[i].y);
            }

            if (alpha >= beta) {
                break;
            }
        }
        return ans;
    }
}

void write_valid_spot(std::ofstream& fout) {
    State a=State(board);
   /* Point ans=search(1,a,player,0,100000);
    fout << ans.x << " " << ans.y << std::endl;
    fout.flush();*/
    Point ans=search(4,a,player,0,100000,fout);
    fout << ans.x << " " << ans.y << std::endl;
    fout.flush();
    
}

int main(int, char** argv) {
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    read_board(fin);
    
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}






