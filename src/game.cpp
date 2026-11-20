
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <cctype>
#include <emscripten/bind.h>

using namespace std;

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

class WordGuess : public Game {
private:
    vector<string> words = {
        "APPLE", "PLANT", "GRAPE", "BRAIN",
        "DREAM", "MUSIC", "LIGHT", "SMILE"
    };

    string answer;
    vector<string> guesses;
    bool finished;

public:
    WordGuess() : Game("Word Guess") {
        newRound();
    }

    void newRound() {
        static mt19937 rng(static_cast<unsigned>(time(nullptr)));
        uniform_int_distribution<int> dist(0, words.size() - 1);

        answer = words[dist(rng)];
        guesses.clear();
        finished = false;
        score = 0;
    }

    string makeGuess(string word) {
        transform(word.begin(), word.end(), word.begin(), ::toupper);

        if (finished) return "Round finished. Start a new round.";
        if (word.length() != 5) return "Enter exactly five letters.";
        if (!all_of(word.begin(), word.end(), ::isalpha))
            return "Use letters only.";

        if (find(guesses.begin(), guesses.end(), word) != guesses.end())
            return "You already tried that word.";

        guesses.push_back(word);

        string result = "";

        for (int i = 0; i < 5; i++) {
            if (word[i] == answer[i])
                result += "G";
            else if (answer.find(word[i]) != string::npos)
                result += "Y";
            else
                result += "X";
        }

        if (word == answer) {
            finished = true;
            score = 100;
            return result + "|You guessed it! You won!";
        }

        if (guesses.size() >= 6) {
            finished = true;
            return result + "|Game over! The word was " + answer;
        }

        return result + "|Try again!";
    }

    int getAttempts() const {
        return guesses.size();
    }

    string getGuessAt(int index) const {
    if (index < 0 || index >= guesses.size())
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
    vector<string> words = {
        "COMPUTER", "GARDEN", "PURPLE",
        "MONSTER", "JOURNEY", "KEYBOARD"
    };

    string answer;
    string guessed;
    int mistakes;
    bool finished;

public:
    Hangman() : Game("Hangman") {
        newRound();
    }

    void newRound() {
        static mt19937 rng(static_cast<unsigned>(time(nullptr)) + 10);
        uniform_int_distribution<int> dist(0, words.size() - 1);

        answer = words[dist(rng)];
        guessed = "";
        mistakes = 0;
        finished = false;
    }

    string guessLetter(string letter) {
        transform(letter.begin(), letter.end(), letter.begin(), ::toupper);

        if (finished) return "Round finished. Start a new round.";
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
    vector<string> words = {
        "FLOWER", "GUITAR", "PLANET",
        "SCHOOL", "FRIEND", "POCKET"
    };

    string answer;
    string scrambled;
    int attempts;
    bool finished;

public:
    WordScramble() : Game("Word Scramble") {
        newRound();
    }

    void newRound() {
        static mt19937 rng(static_cast<unsigned>(time(nullptr)) + 20);
        uniform_int_distribution<int> dist(0, words.size() - 1);

        answer = words[dist(rng)];
        scrambled = answer;

        shuffle(scrambled.begin(), scrambled.end(), rng);

        if (scrambled == answer)
            rotate(scrambled.begin(), scrambled.begin() + 1,
                   scrambled.end());

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
        transform(guess.begin(), guess.end(), guess.begin(), ::toupper);

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
        .constructor<>()
        .function("newRound", &WordGuess::newRound)
        .function("makeGuess", &WordGuess::makeGuess)
        .function("getAttempts", &WordGuess::getAttempts)
        .function("getGuessAt", &WordGuess::getGuessAt)
        .function("isFinished", &WordGuess::isFinished);

    emscripten::class_<Hangman>("Hangman")
        .constructor<>()
        .function("newRound", &Hangman::newRound)
        .function("guessLetter", &Hangman::guessLetter)
        .function("getDisplayWord", &Hangman::getDisplayWord)
        .function("getMistakes", &Hangman::getMistakes);

    emscripten::class_<TicTacToe>("TicTacToe")
        .constructor<>()
        .function("newRound", &TicTacToe::newRound)
        .function("makeMove", &TicTacToe::makeMove)
        .function("getCell", &TicTacToe::getCell);

    emscripten::class_<WordScramble>("WordScramble")
        .constructor<>()
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
