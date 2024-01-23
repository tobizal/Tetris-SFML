#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <random>
#include <iostream>
#include <fstream>
#include <time.h>

const int W = 20;
const int H = 30;
const int scaleFactor = 1;
const int unit = 18*scaleFactor;
const int WIDTH = W*unit;
const int HEIGHT = H*unit;
const float init_delay = 0.7;
const float delayModPerBlockStackFallen = 0.85;
const float scoreBoostPerBlockStackFallen = 5;
const int blockStack = 7;

long score{};
long highscore = -1;
int pointsPerLayer = 10;
sf::Text scoreText;
sf::Text highscoreText;

sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Tetris");
sf::Texture tiles_texture;
sf::Font font;
sf::Music music;
sf::Event event;

enum Color{Blue=0, Purple, Red, Green, Yellow, Lightblue, Orange, Darkblue, Blank};
enum Shape{Stick=0, Square, T, Z, Z_, L, L_};
enum Direction{Left=-1, Right=1};

Color field[W][H]{Blank};
const int pattern[7][4] = {
    0, 2, 4, 6, //I
    0, 1, 2, 3, //square
    0, 2, 3, 4, //T
    1, 2, 3, 4, //Z
    0, 2, 3, 5, //Z prime
    0, 2, 4, 5, //L
    1, 3, 4, 5  //L prime
    
};

int randInt(int lower, int upper){
    std::random_device dev;
    std::default_random_engine generator(dev());
    std::uniform_int_distribution<int> distr(lower, upper);
    int r = distr(generator);
    return r;
}

void delay(int milliseconds){
    clock_t time = clock();
    while((float)(clock()-time)/CLOCKS_PER_SEC*1000<milliseconds){
        continue;
    }
}

void loadGraphicsAndAudio(){
    if (!tiles_texture.loadFromFile("Resources/tiles.png")) {
        exit(EXIT_FAILURE);
    }
    if (!music.openFromFile("Resources/Tetris_theme.ogg")) {
        exit(EXIT_FAILURE);
    }
    if (!font.loadFromFile("Resources/sansation.ttf")) {
        exit(EXIT_FAILURE);
    }
}

void loadHighScore(){
    std::fstream plik("Resources/highscore.txt", std::ios::in);
    if(!plik.good()){
        return;
    }
    plik>>highscore;
    plik.close();
}
void saveHighscore(){
    std::fstream plik("Resources/highscore.txt", std::ios::out);
    plik<<highscore;
    plik.close();
}


class Tile: public sf::Sprite{
public:
    Color color;
    Tile(Color c): color(c){
        sf::Sprite::setTexture(tiles_texture);
        sf::Sprite::setTextureRect({c*18, 0, 18, 18});
        sf::Sprite::scale(scaleFactor, scaleFactor);
    }
    Tile()=default;
};

class Block{
    Tile tile[4];
    Shape shape;
    Color color;
public:
    Block(){
        shape = static_cast<Shape>(randInt(0, 6));
        color = static_cast<Color>(randInt(0, 6));
        for(int i=0; i<4; i++){
            Tile t(color);
            tile[i] = t;
            tile[i].setPosition((pattern[shape][i]%2)*unit + WIDTH/2-unit, (pattern[shape][i]/2)*unit - unit);
        }
    }
    void draw(){
        for(int i=0; i<4; i++)
            window.draw(tile[i]);
    }
    void move(Direction d){
        float dx = d*unit;
        for(int i=0; i<4; i++){
            int newPositionX = tile[i].getPosition().x + dx;
            if(newPositionX > WIDTH - unit || newPositionX < 0) return;
        }
        for(int i=0; i<4; i++)
            tile[i].move({static_cast<float>(d)*unit,0});
    }
    void rotate(){
        sf::Vector2f center = tile[1].getPosition();
        for(int i=0; i<4; i++){
            int x = tile[i].getPosition().x;
            int y = tile[i].getPosition().y;
            int dx = x - center.x;
            int dy = y - center.y;
            
            tile[i].setPosition(center.x-dy, center.y+dx);
        }
        for(int i=0; i<4; i++){
            while(tile[i].getPosition().x >= WIDTH){
                for(int i=0; i<4; i++)
                    tile[i].move({-unit,0});
            }
            while(tile[i].getPosition().x < 0){
                for(int i=0; i<4; i++)
                    tile[i].move({unit,0});
            }
        }
    }
    void dropDown(){
        for(int i=0; i<4; i++)
            tile[i].move({0, unit});
    }
    void goUp(){
        for(int i=0; i<4; i++)
            tile[i].move({0, -unit});
    }
    bool cantFallFurther(){
        for(int i=0; i<4; i++){
            int x = tile[i].getPosition().x / unit;
            int y = tile[i].getPosition().y / unit;
            
            if(x >= 0 && y >=0 && field[x][y] != Blank ) return true;
            if(tile[i].getPosition().y >= HEIGHT) return true;
        }
        return false;
    }
    void addToField(){
        for(int i=0; i<4; i++){
            int x = tile[i].getPosition().x / unit;
            int y = tile[i].getPosition().y / unit;
            field[x][y] = color;
        }
    }
    bool didOverflow(){
        for(int i=0; i<4; i++){
            if(tile[i].getPosition().y <= 0){
                std::cout<<"Position y: "<<tile[0].getPosition().y<<" "<<tile[1].getPosition().y<<" "<<tile[2].getPosition().y<<" "<<tile[3].getPosition().y<<" "<<std::endl;
                return true;
            }
        }
        return false;
    }
};

void initializeField(){
    for(int i=0; i<W;i++){
        for(int j=0; j<H; j++){
            field[i][j]=Blank;
        }
    }
}

void drawField(){
    for(int i=0; i<W; i++){
        for(int j=0; j<H; j++){
            Color c = field[i][j];
            if(c==Blank) continue;
            Tile t(c);
            t.setPosition(i*unit, j*unit);
            window.draw(t);
        }
    }
}

void rowScored(int* rows, int size){
    if(size==0) return;
    for(int k=0; k<10; k++){
        for(int i=0; i<W; i++){
            for(int r=0; r<size; r++){
                field[i][rows[r]] = static_cast<Color>(randInt(0, 6));
            }
        }
        drawField();
        window.display();
        delay(90);
    }
    for(int r=0; r<size; r++){
        for(int j=rows[r]; j>0; j--){
            for(int i=0; i<W; i++){
                field[i][j] = field[i][j-1];
            }
        }
        for(int i=0; i<W; i++){
            field[i][0] = Blank;
        }
    }
    
    score += size*size*pointsPerLayer;
    scoreText.setString("Score: " + std::to_string(score));
}

void checkFieldForLayers(){
    int rowsThatWhereScored[4]{};
    int counter{};
    for(int j=0; j<H; j++){
        bool fullRow = true;
        for(int i=0; i<W; i++){
            if(field[i][j] == Blank){
                fullRow = false;
                break;
            }
        }
        if(fullRow){
            rowsThatWhereScored[counter] = j;
            counter++;
        }
    }
    rowScored(rowsThatWhereScored, counter);
}

void endscreen(){
    if(score>highscore) highscore = score;
    saveHighscore();
    char c;
    sf::Text t1;
    sf::Text t2;
    sf::Text t3;
    t1.setFont(font);
    t1.setString("Your score: " + std::to_string(score));
    t1.setPosition(2*unit, HEIGHT/2 - 4*unit);
    t1.setScale(0.6, 0.6);
    t2.setFont(font);
    t2.setString("Current Highscore: " + std::to_string(highscore));
    t2.setPosition(2*unit, HEIGHT/2 );
    t2.setScale(0.6, 0.6);
    t3.setFont(font);
    t3.setString("Press escape to close the window...");
    t3.setPosition(2*unit, HEIGHT/2+2*unit);
    t3.setScale(0.6, 0.6);
    window.draw(t1);
    window.draw(t2);
    window.draw(t3);
    window.display();
    while(window.isOpen()){
        while (window.pollEvent(event)){
            if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape){
                window.close();
            }
        }
    }
}

int main(int argc, char const** argv){
    loadHighScore();
    initializeField();
    loadGraphicsAndAudio();
    
    scoreText.setFont(font);
    scoreText.setScale(0.6, 0.6);
    scoreText.setPosition(unit, unit/2);
    scoreText.setString("Score: " + std::to_string(score));
    highscoreText.setFont(font);
    highscoreText.setScale(0.6, 0.6);
    highscoreText.setPosition(unit, 3*unit/2);
    highscoreText.setString("Highscore: " + std::to_string(highscore));
    Block* b;
    b = new Block;
    float delay = init_delay;
    float modifier = 1;
    int blocksFallenCount = 1;
    music.play();
    music.setLoop(true);
    clock_t time = clock();

    while (window.isOpen()){
        while (window.pollEvent(event)){
            if(event.type == sf::Event::KeyPressed){
                switch(event.key.code){
                    case sf::Keyboard::D:[[fallthrough]];
                    case sf::Keyboard::Right:
                        b->move(Direction::Right); break;
                    case sf::Keyboard::A:[[fallthrough]];
                    case sf::Keyboard::Left:
                        b->move(Direction::Left); break;
                    case sf::Keyboard::S:[[fallthrough]];
                    case sf::Keyboard::Down:
                        delay = 0.05; break;
                    case sf::Keyboard::W:[[fallthrough]];
                    case sf::Keyboard::Up:
                        b->rotate(); break;
                    case sf::Keyboard::Escape:
                        window.close(); break;
                    default: break;
                }
            }
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        
        float dt = (float)(clock() - time)/CLOCKS_PER_SEC;
        if(dt > delay){
            b->dropDown();
            time = clock();
        }
        
        if(b->cantFallFurther()){
            b->goUp();
            if(b->didOverflow()){
                endscreen();
            }
            b->addToField();
            b = new Block();
            blocksFallenCount++;
            if(blocksFallenCount % blockStack == 0) {
                modifier *= delayModPerBlockStackFallen;
                pointsPerLayer += scoreBoostPerBlockStackFallen;
            }
            checkFieldForLayers();
        }
        
        //resetting variables
        delay = init_delay*modifier;
        
        // clearing screen and updating the window
        window.clear();
        drawField();
        b->draw();
        window.draw(scoreText);
        window.draw(highscoreText);
        window.display();
    }

    return EXIT_SUCCESS;
}


