
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <cctype>
#include <fstream>
#include <unordered_set>
#include <emscripten/bind.h>

using namespace std;

const vector<string> COMMON_WORDS = {
    "APPLE", "BEACH", "BREAD", "BRUSH", "CHAIR",
    "CLOUD", "DANCE", "DREAM", "EARTH", "FRUIT",
    "GLASS", "GRAPE", "GREEN", "HAPPY", "HEART",
    "HOUSE", "JUICE", "LIGHT", "MONEY", "MOUSE",
    "MUSIC", "NIGHT", "OCEAN", "PAPER", "PARTY",
    "PHONE", "PLANT", "PLATE", "PIZZA", "QUEEN",
    "RIVER", "SHEEP", "SMILE", "SNAKE", "SOUND",
    "SPACE", "SPOON", "STONE", "STORM", "TABLE",
    "TIGER", "TRAIN", "WATER", "WORLD", "YOUNG",
    "BASKET", "BOTTLE", "BUTTER", "CANDLE", "CARROT",
    "CASTLE", "CHERRY", "COOKIE", "FLOWER", "GARDEN",
    "GUITAR", "HAMMER", "JACKET", "KITTEN", "LITTLE",
    "MONKEY", "ORANGE", "PENCIL", "PILLOW", "POCKET",
    "RABBIT", "SCHOOL", "SILVER", "SISTER", "SQUARE",
    "SUMMER", "TURTLE", "WINDOW"
};

const vector<string> ADVANCED_WORDS = {
    "ABHOR", "ADEPT", "AMASS", "BANAL", "BERTH",
    "BLURT", "BRISK", "CHASM", "CLOUT", "COVET",
    "CRYPT", "DAUNT", "DELVE", "ELUDE", "ENVOY",
    "EVOKE", "FEIGN", "FOIST", "GAUDY", "GLEAN",
    "GLOAT", "GNASH", "GUISE", "HORDE", "INERT",
    "KNEAD", "LURCH", "LURID", "MIDGE", "MIRTH",
    "NICHE", "OPINE", "PLAIT", "PLUCK", "PRONG",
    "QUIRK", "RAVEL", "REBUT", "REIGN", "SCOUR",
    "SHREW", "SIEVE", "SKULK", "SMIRK", "SPURN",
    "STOIC", "TAUNT", "TERSE", "VAUNT", "VIGOR",
    "WIELD", "WRING", "YEARN"
};

class Game {
protected:
    string gameName;
    int score;

public:
    Game(string name) : gameName(name), score(0) {}

    virtual string getName() const {
        return gameName;
    }

    virtual string getStatus() const = 0;

    virtual ~Game() {}
};

class WordPicker {
private:
    vector<string> words;
    vector<string> shuffledWords;
    size_t nextWord;
    string lastWord;
    mt19937 rng;

public:
    WordPicker(int difficulty, int minLength, int maxLength)
        : nextWord(0),
          rng(static_cast<unsigned>(time(nullptr)) +
              minLength * 100 + maxLength * 10 + difficulty) {

        if (difficulty == 0) {
            for (const string& word : COMMON_WORDS) {
                if (word.length() >= static_cast<size_t>(minLength) &&
                    word.length() <= static_cast<size_t>(maxLength)) {
                    words.push_back(word);
                }
            }
        }
        else {
            ifstream file("/words_alpha.txt");
            string entry;
            unordered_set<string> uniqueWords;

            while (getline(file, entry)) {
                entry.erase(remove(entry.begin(), entry.end(), '\r'),
                    entry.end());

                transform(entry.begin(), entry.end(), entry.begin(),
                    [](unsigned char c) {
                        return toupper(c);
                    });

                if (entry.length() >= static_cast<size_t>(minLength) &&
                    entry.length() <= static_cast<size_t>(maxLength) &&
                    all_of(entry.begin(), entry.end(),
                        [](unsigned char c) {
                            return isalpha(c);
                        })) {

                    if (uniqueWords.insert(entry).second) {
                        words.push_back(entry);
                    }
                }
            }
        }

        if (words.empty()) {
            for (const string& word : COMMON_WORDS) {
                if (word.length() >= static_cast<size_t>(minLength) &&
                    word.length() <= static_cast<size_t>(maxLength)) {
                    words.push_back(word);
                }
            }

            for (const string& word : ADVANCED_WORDS) {
                if (word.length() >= static_cast<size_t>(minLength) &&
                    word.length() <= static_cast<size_t>(maxLength)) {
                    words.push_back(word);
                }
            }
        }

        shuffle(words.begin(), words.end(), rng);
        shuffledWords = words;
    }

    string getNextWord() {
        if (shuffledWords.empty()) {
            return "GARDEN";
        }

        if (nextWord >= shuffledWords.size()) {
            shuffledWords = words;
            shuffle(shuffledWords.begin(), shuffledWords.end(), rng);
            nextWord = 0;

            if (shuffledWords.size() > 1 &&
                shuffledWords[0] == lastWord) {
                swap(shuffledWords[0], shuffledWords[1]);
            }
        }

        string selected = shuffledWords[nextWord++];
        lastWord = selected;

        return selected;
    }
};

class WordGuess : public Game {
private:
    vector<string> answerPool;
    vector<string> shuffledAnswers;
    vector<string> guesses;
    unordered_set<string> validWords;

    size_t nextAnswer;
    string answer;
    string lastAnswer;
    bool finished;
    int difficulty;
    int maxAttempts;

    void loadDictionary() {
        ifstream file("/words.txt");
        string entry;
        unordered_set<string> uniqueWords;

        while (getline(file, entry)) {
            entry.erase(remove(entry.begin(), entry.end(), '\r'),
                entry.end());

            transform(entry.begin(), entry.end(), entry.begin(),
                [](unsigned char c) {
                    return toupper(c);
                });

            if (entry.length() == 5 &&
                all_of(entry.begin(), entry.end(),
                    [](unsigned char c) {
                        return isalpha(c);
                    })) {

                if (uniqueWords.insert(entry).second) {
                    validWords.insert(entry);

                    bool common = find(
                        COMMON_WORDS.begin(),
                        COMMON_WORDS.end(),
                        entry
                    ) != COMMON_WORDS.end();

                    if (difficulty == 0 && common) {
                        answerPool.push_back(entry);
                    }
                    else if (difficulty == 1) {
                        answerPool.push_back(entry);
                    }
                    else if (difficulty == 2 && !common) {
                        answerPool.push_back(entry);
                    }
                }
            }
        }

        for (const string& word : COMMON_WORDS) {
            if (word.length() == 5) {
                validWords.insert(word);

                if (difficulty == 0 &&
                    find(answerPool.begin(), answerPool.end(), word)
                    == answerPool.end()) {
                    answerPool.push_back(word);
                }
            }
        }

        if (difficulty == 2 && answerPool.empty()) {
            for (const string& word : ADVANCED_WORDS) {
                if (word.length() == 5) {
                    answerPool.push_back(word);
                    validWords.insert(word);
                }
            }
        }

        if (answerPool.empty()) {
            answerPool = COMMON_WORDS;
        }

        shuffledAnswers = answerPool;
        nextAnswer = shuffledAnswers.size();
    }

    void selectAnswer() {
        static mt19937 rng(static_cast<unsigned>(time(nullptr)));

        if (answerPool.empty()) {
            answer = "PLANT";
            return;
        }

        if (nextAnswer >= shuffledAnswers.size()) {
            shuffledAnswers = answerPool;
            shuffle(shuffledAnswers.begin(), shuffledAnswers.end(), rng);
            nextAnswer = 0;

            if (shuffledAnswers.size() > 1 &&
                shuffledAnswers[0] == lastAnswer) {
                swap(shuffledAnswers[0], shuffledAnswers[1]);
            }
        }

        answer = shuffledAnswers[nextAnswer++];
        lastAnswer = answer;
    }

public:
    WordGuess(int level)
        : Game("Word Guess"),
          nextAnswer(0),
          finished(false),
          difficulty(level),
          maxAttempts(level == 0 ? 8 : (level == 1 ? 6 : 4)) {

        loadDictionary();
        newRound();
    }

    void newRound() {
        selectAnswer();
        guesses.clear();
        finished = false;
        score = 0;
    }

    string makeGuess(string word) {
        transform(word.begin(), word.end(), word.begin(),
            [](unsigned char c) {
                return toupper(c);
            });

        if (finished)
            return "Round finished. Start a new round.";

        if (word.length() != 5)
            return "Enter exactly five letters.";

        if (!all_of(word.begin(), word.end(),
            [](unsigned char c) {
                return isalpha(c);
            }))
            return "Use letters only.";

        if (validWords.find(word) == validWords.end())
            return "Not a valid word. Try again.";

        if (find(guesses.begin(), guesses.end(), word)
            != guesses.end())
            return "You already tried that word.";

        guesses.push_back(word);

        string result = "";

        for (int i = 0; i < 5; i++) {
            if (word[i] == answer[i]) {
                result += "G";
            }
            else if (answer.find(word[i]) != string::npos) {
                result += "Y";
            }
            else {
                result += "X";
            }
        }

        if (word == answer) {
            finished = true;
            score = 100;
            return result + "|You guessed it! You won!";
        }

        if (guesses.size() >= static_cast<size_t>(maxAttempts)) {
            finished = true;
            return result + "|Game over! The word was " + answer;
        }

        return result + "|Try again!";
    }

    int getAttempts() const {
        return guesses.size();
    }

    int getMaxAttempts() const {
        return maxAttempts;
    }

    string getGuessAt(int index) const {
        if (index < 0 || index >= static_cast<int>(guesses.size()))
            return "";

        return guesses[index];
    }

    bool isFinished() const {
        return finished;
    }

    string getStatus() const override {
        return finished ? "Finished" : "Playing";
    }
};

class Hangman : public Game {
private:
    WordPicker wordPicker;
    string answer;
    string guessed;
    int mistakes;
    bool finished;

public:
    Hangman(int difficulty)
        : Game("Hangman"),
          wordPicker(
              difficulty,
              difficulty == 0 ? 4 : (difficulty == 1 ? 6 : 8),
              difficulty == 0 ? 6 : (difficulty == 1 ? 7 : 10)
          ) {
        newRound();
    }

    void newRound() {
        answer = wordPicker.getNextWord();
        guessed = "";
        mistakes = 0;
        finished = false;
        score = 0;
    }

    string guessLetter(string letter) {
        transform(letter.begin(), letter.end(), letter.begin(),
            [](unsigned char c) { return toupper(c); });

        if (finished)
            return "Round finished. Start a new round.";

        if (letter.length() != 1 || !isalpha(letter[0]))
            return "Enter one letter.";

        char ch = letter[0];

        if (guessed.find(ch) != string::npos)
            return "You already guessed that letter.";

        guessed += ch;

        if (answer.find(ch) == string::npos)
            mistakes++;

        if (mistakes >= 6)
            finished = true;

        if (isWordComplete()) {
            finished = true;
            score = 100;
            return "You won! The word was " + answer;
        }

        if (finished)
            return "You lost! The word was " + answer;

        return "Letter accepted.";
    }

    string getDisplayWord() const {
        string display = "";

        for (char ch : answer) {
            if (guessed.find(ch) != string::npos)
                display += ch;
            else
                display += '_';

            display += ' ';
        }

        return display;
    }

    bool isWordComplete() const {
        for (char ch : answer) {
            if (guessed.find(ch) == string::npos)
                return false;
        }

        return true;
    }

    int getMistakes() const {
        return mistakes;
    }

    bool isFinished() const {
        return finished;
    }

    string getStatus() const override {
        return finished ? "Finished" : "Playing";
    }
};

class TicTacToe : public Game {
private:
    vector<string> board;
    string currentPlayer;
    bool finished;
    string result;

    bool checkWin(string player) {
        int lines[8][3] = {
            {0,1,2}, {3,4,5}, {6,7,8},
            {0,3,6}, {1,4,7}, {2,5,8},
            {0,4,8}, {2,4,6}
        };

        for (auto &line : lines) {
            if (board[line[0]] == player &&
                board[line[1]] == player &&
                board[line[2]] == player)
                return true;
        }

        return false;
    }

public:
    TicTacToe() : Game("Tic-Tac-Toe") {
        newRound();
    }

    void newRound() {
        board = vector<string>(9, "");
        currentPlayer = "X";
        finished = false;
        result = "Player X's turn";
    }

    string makeMove(int position) {
        if (finished) return "Game finished. Start a new match.";

        if (position < 0 || position > 8)
            return "Invalid position.";

        if (board[position] != "")
            return "That position is occupied.";

        board[position] = currentPlayer;

        if (checkWin(currentPlayer)) {
            finished = true;
            result = "Player " + currentPlayer + " wins!";
            score = 100;
            return result;
        }

        if (all_of(board.begin(), board.end(),
            [](const string &cell) { return cell != ""; })) {
            finished = true;
            result = "It's a draw!";
            return result;
        }

        currentPlayer = currentPlayer == "X" ? "O" : "X";
        result = "Player " + currentPlayer + "'s turn";

        return result;
    }

    string getCell(int position) const {
        if (position < 0 || position > 8) return "";
        return board[position];
    }

    string getStatus() const override {
        return finished ? "Finished" : "Playing";
    }
};

class WordScramble : public Game {
private:
    WordPicker wordPicker;
    string answer;
    string scrambled;
    int attempts;
    bool finished;

public:
    WordScramble(int difficulty)
        : Game("Word Scramble"),
          wordPicker(
              difficulty,
              difficulty == 0 ? 5 : (difficulty == 1 ? 6 : 8),
              difficulty == 0 ? 6 : (difficulty == 1 ? 7 : 10)
          ) {
        newRound();
    }

    void newRound() {
        answer = wordPicker.getNextWord();
        scrambled = answer;

        static mt19937 rng(
            static_cast<unsigned>(time(nullptr)) + 20
        );

        shuffle(scrambled.begin(), scrambled.end(), rng);

        if (scrambled == answer && scrambled.length() > 1) {
            rotate(scrambled.begin(), scrambled.begin() + 1,
                scrambled.end());
        }

        attempts = 0;
        finished = false;
        score = 0;
    }

    string getScrambledWord() const {
        return scrambled;
    }

    string getHint() const {
        return "The word starts with " + answer.substr(0, 1) +
            " and has " + to_string(answer.length()) + " letters.";
    }

    string submitAnswer(string guess) {
        transform(guess.begin(), guess.end(), guess.begin(),
            [](unsigned char c) { return toupper(c); });

        if (finished) return "Round finished. Start a new round.";

        attempts++;

        if (guess == answer) {
            finished = true;
            score = max(10, 100 - (attempts - 1) * 10);
            return "Correct! You solved it!";
        }

        return "Incorrect! Try again.";
    }

    int getScore() const {
        return score;
    }

    string getStatus() const override {
        return finished ? "Finished" : "Playing";
    }
};

class NumberGuess : public Game {
private:
    int answer;
    int attempts;
    bool finished;

public:
    NumberGuess() : Game("Number Guess") {
        newRound();
    }

    void newRound() {
        static mt19937 rng(static_cast<unsigned>(time(nullptr)) + 30);
        uniform_int_distribution<int> dist(1, 100);

        answer = dist(rng);
        attempts = 0;
        finished = false;
        score = 0;
    }

    string makeGuess(int number) {
        if (finished) return "Round finished. Start a new round.";

        if (number < 1 || number > 100)
            return "Enter a number between 1 and 100.";

        attempts++;

        if (number == answer) {
            finished = true;
            score = max(10, 100 - (attempts - 1) * 10);
            return "Correct! You guessed the number!";
        }

        if (number < answer)
            return "Too low! Try a higher number.";

        return "Too high! Try a lower number.";
    }

    int getAttempts() const {
        return attempts;
    }

    int getScore() const {
        return score;
    }

    string getStatus() const override {
        return finished ? "Finished" : "Playing";
    }
};

EMSCRIPTEN_BINDINGS(playroom_module) {
    emscripten::class_<WordGuess>("WordGuess")
        .constructor<int>()
        .function("newRound", &WordGuess::newRound)
        .function("makeGuess", &WordGuess::makeGuess)
        .function("getAttempts", &WordGuess::getAttempts)
        .function("getMaxAttempts", &WordGuess::getMaxAttempts)
        .function("getGuessAt", &WordGuess::getGuessAt)
        .function("isFinished", &WordGuess::isFinished);

    emscripten::class_<Hangman>("Hangman")
        .constructor<int>()
        .function("newRound", &Hangman::newRound)
        .function("guessLetter", &Hangman::guessLetter)
        .function("getDisplayWord", &Hangman::getDisplayWord)
        .function("getMistakes", &Hangman::getMistakes)
        .function("isFinished", &Hangman::isFinished);

    emscripten::class_<TicTacToe>("TicTacToe")
        .constructor<>()
        .function("newRound", &TicTacToe::newRound)
        .function("makeMove", &TicTacToe::makeMove)
        .function("getCell", &TicTacToe::getCell);

    emscripten::class_<WordScramble>("WordScramble")
        .constructor<int>()
        .function("newRound", &WordScramble::newRound)
        .function("getScrambledWord", &WordScramble::getScrambledWord)
        .function("getHint", &WordScramble::getHint)
        .function("submitAnswer", &WordScramble::submitAnswer)
        .function("getScore", &WordScramble::getScore);

    emscripten::class_<NumberGuess>("NumberGuess")
        .constructor<>()
        .function("newRound", &NumberGuess::newRound)
        .function("makeGuess", &NumberGuess::makeGuess)
        .function("getAttempts", &NumberGuess::getAttempts)
        .function("getScore", &NumberGuess::getScore);
}
