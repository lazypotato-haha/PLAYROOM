
import createPlayroomModule from "./build/game.js";

let Module;
let currentGame = null;
let activeGame = "";
let wordInput = "";
let wordFeedback = [];
let hangmanGuessed = [];

const homeScreen = document.getElementById("home-screen");
const gameScreen = document.getElementById("game-screen");
const gameContent = document.getElementById("game-content");

async function initializePlayroom() {
    try {
        Module = await createPlayroomModule();
        console.log("PLAYROOM C++ engine loaded successfully.");
    } catch (error) {
        console.error("Could not load the C++ engine:", error);
        gameContent.innerHTML = "<p>Unable to load the games. Please refresh the page.</p>";
    }
}

initializePlayroom();

function openGame(game) {
    if (!Module) {
        alert("The games are still loading. Please try again.");
        return;
    }

    if (currentGame) {
        currentGame.delete();
        currentGame = null;
    }

    activeGame = game;
    homeScreen.classList.remove("active");
    gameScreen.classList.add("active");

    switch (game) {
        case "word":
            currentGame = new Module.WordGuess();
            showWordGuess();
            break;
        case "hangman":
            currentGame = new Module.Hangman();
            hangmanGuessed = [];
            showHangman();
            break;
        case "tic":
            currentGame = new Module.TicTacToe();
            showTicTacToe();
            break;
        case "scramble":
            currentGame = new Module.WordScramble();
            showScramble();
            break;
        case "number":
            currentGame = new Module.NumberGuess();
            showNumberGuess();
            break;
    }
}

function goHome() {
    activeGame = "";
    window.onkeydown = null;
    gameScreen.classList.remove("active");
    homeScreen.classList.add("active");

    if (currentGame) {
        currentGame.delete();
        currentGame = null;
    }
}

// WORD GUESS

function showWordGuess() {
    wordInput = "";
    wordFeedback = [];

    gameContent.innerHTML = `
        <h2>Word Guess</h2>
        <p>Guess the hidden five-letter word in six attempts.</p>
        <div id="word-board"></div>

        <div id="word-keyboard">
            <div class="keyboard-row">
                ${"QWERTYUIOP".split("").map(letter =>
                    `<button class="key" onclick="addWordLetter('${letter}')" id="key-${letter}">${letter}</button>`
                ).join("")}
            </div>
            <div class="keyboard-row">
                ${"ASDFGHJKL".split("").map(letter =>
                    `<button class="key" onclick="addWordLetter('${letter}')" id="key-${letter}">${letter}</button>`
                ).join("")}
            </div>
            <div class="keyboard-row">
                <button class="key special-key" onclick="submitWordGuess()">ENTER</button>
                ${"ZXCVBNM".split("").map(letter =>
                    `<button class="key" onclick="addWordLetter('${letter}')" id="key-${letter}">${letter}</button>`
                ).join("")}
                <button class="key special-key" onclick="removeWordLetter()">⌫</button>
            </div>
        </div>

        <p id="word-message">Start guessing!</p>
        <button onclick="restartWord()">Play Again</button>
    `;

    updateWordBoard();
    window.onkeydown = handleWordKeyboard;
}

function addWordLetter(letter) {
    if (activeGame === "word" && wordInput.length < 5 &&
        !currentGame.isFinished()) {
        wordInput += letter;
        updateWordBoard();
    }
}

function removeWordLetter() {
    if (activeGame === "word" && wordInput.length > 0) {
        wordInput = wordInput.slice(0, -1);
        updateWordBoard();
    }
}

function handleWordKeyboard(event) {
    if (activeGame !== "word" || !currentGame) return;

    if (event.key === "Enter") {
        submitWordGuess();
    } else if (event.key === "Backspace") {
        removeWordLetter();
    } else if (/^[a-zA-Z]$/.test(event.key)) {
        addWordLetter(event.key.toUpperCase());
    }
}

function updateWordBoard() {
    const board = document.getElementById("word-board");
    if (!board) return;

    board.innerHTML = "";
    const attempts = currentGame.getAttempts();

    for (let i = 0; i < 6; i++) {
        const row = document.createElement("div");
        row.className = "word-row";

        let guess = "";
        let feedback = "";

        if (i < attempts) {
            guess = currentGame.getGuessAt(i);
            feedback = wordFeedback[i] || "";
        } else if (i === attempts && !currentGame.isFinished()) {
            guess = wordInput;
        }

        for (let j = 0; j < 5; j++) {
            const tile = document.createElement("span");
            tile.className = "word-tile";
            tile.textContent = guess[j] || "";

            if (feedback[j] === "G") tile.classList.add("green");
            if (feedback[j] === "Y") tile.classList.add("yellow");
            if (feedback[j] === "X") tile.classList.add("grey");

            row.appendChild(tile);
        }

        board.appendChild(row);
    }

    updateWordKeyboard();
}

function updateWordKeyboard() {
    const keyboardStatus = {};
    const priority = { X: 1, Y: 2, G: 3 };

    for (let i = 0; i < wordFeedback.length; i++) {
        const guess = currentGame.getGuessAt(i);
        const feedback = wordFeedback[i];

        for (let j = 0; j < 5; j++) {
            const letter = guess[j];
            const result = feedback[j];

            if (!keyboardStatus[letter] ||
                priority[result] > priority[keyboardStatus[letter]]) {
                keyboardStatus[letter] = result;
            }
        }
    }

    Object.keys(keyboardStatus).forEach(letter => {
        const key = document.getElementById(`key-${letter}`);

        if (key) {
            key.classList.remove("green", "yellow", "grey");

            if (keyboardStatus[letter] === "G") key.classList.add("green");
            if (keyboardStatus[letter] === "Y") key.classList.add("yellow");
            if (keyboardStatus[letter] === "X") key.classList.add("grey");
        }
    });
}

function submitWordGuess() {
    if (activeGame !== "word" || !currentGame) return;

    if (wordInput.length !== 5) {
        document.getElementById("word-message").textContent =
            "Enter all five letters first!";
        return;
    }

    const result = currentGame.makeGuess(wordInput);
    const parts = result.split("|");

    if (parts.length < 2) {
        document.getElementById("word-message").textContent = result;
        return;
    }

    wordFeedback.push(parts[0]);
    wordInput = "";

    document.getElementById("word-message").textContent = parts[1];
    updateWordBoard();
}

function restartWord() {
    currentGame.newRound();
    showWordGuess();
}

// HANGMAN

function showHangman() {
    hangmanGuessed = [];

    gameContent.innerHTML = `
        <h2>Hangman</h2>
        <p>Guess the hidden word before the hangman is complete!</p>

        <div id="hangman-drawing"></div>

        <h2 id="hangman-word"></h2>

        <p class="hangman-counter">
            Mistakes: <strong id="mistakes">0</strong> / 6
        </p>

        <div id="hangman-keyboard">
            ${"ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("").map(letter =>
                `<button class="hangman-key" id="hang-key-${letter}"
                    onclick="guessHangmanLetter('${letter}')">${letter}</button>`
            ).join("")}
        </div>

        <p id="hangman-message">Choose a letter to begin!</p>
        <button onclick="restartHangman()">Play Again</button>
    `;

    updateHangman();
    window.onkeydown = handleHangmanKeyboard;
}

function getHangmanDrawing(mistakes) {
    return `
        <svg viewBox="0 0 220 220" class="hangman-svg">
            <line x1="25" y1="200" x2="150" y2="200"/>
            <line x1="65" y1="200" x2="65" y2="20"/>
            <line x1="65" y1="20" x2="155" y2="20"/>
            <line x1="155" y1="20" x2="155" y2="45"/>

            ${mistakes >= 1 ? '<circle cx="155" cy="65" r="20"/>' : ""}
            ${mistakes >= 2 ? '<line x1="155" y1="85" x2="155" y2="140"/>' : ""}
            ${mistakes >= 3 ? '<line x1="155" y1="100" x2="125" y2="125"/>' : ""}
            ${mistakes >= 4 ? '<line x1="155" y1="100" x2="185" y2="125"/>' : ""}
            ${mistakes >= 5 ? '<line x1="155" y1="140" x2="125" y2="175"/>' : ""}
            ${mistakes >= 6 ? '<line x1="155" y1="140" x2="185" y2="175"/>' : ""}
        </svg>
    `;
}

function updateHangman() {
    if (!currentGame) return;

    const mistakes = currentGame.getMistakes();
    const displayWord = currentGame.getDisplayWord();

    document.getElementById("hangman-word").textContent = displayWord;
    document.getElementById("mistakes").textContent = mistakes;
    document.getElementById("hangman-drawing").innerHTML =
        getHangmanDrawing(mistakes);

    hangmanGuessed.forEach(letter => {
        const key = document.getElementById(`hang-key-${letter}`);
        if (key) key.disabled = true;
    });
}

function guessHangmanLetter(letter) {
    if (activeGame !== "hangman" || !currentGame) return;

    if (hangmanGuessed.includes(letter) || currentGame.getMistakes() >= 6) {
        return;
    }

    hangmanGuessed.push(letter);

    const result = currentGame.guessLetter(letter);
    const key = document.getElementById(`hang-key-${letter}`);

    if (key) {
        key.disabled = true;

        if (result.includes("already")) {
            key.classList.remove("correct-key", "wrong-key");
            return;
        }

        if (currentGame.getDisplayWord().includes(letter)) {
            key.classList.add("correct-key");
        } else {
            key.classList.add("wrong-key");
        }
    }

    document.getElementById("hangman-message").textContent = result;
    updateHangman();
}

function handleHangmanKeyboard(event) {
    if (activeGame !== "hangman") return;

    if (/^[a-zA-Z]$/.test(event.key)) {
        guessHangmanLetter(event.key.toUpperCase());
    }
}

function restartHangman() {
    currentGame.newRound();
    showHangman();
}

// TIC-TAC-TOE

function showTicTacToe() {
    gameContent.innerHTML = `
        <h2>Tic-Tac-Toe</h2>
        <p id="tic-message">Player X's turn</p>

        <div class="tic-board">
            ${Array.from({ length: 9 }, (_, i) =>
                `<button class="tic-cell" onclick="makeTicMove(${i})" id="cell-${i}"></button>`
            ).join("")}
        </div>

        <button onclick="restartTic()">Play Again</button>
    `;
}

function makeTicMove(position) {
    const result = currentGame.makeMove(position);

    for (let i = 0; i < 9; i++) {
        document.getElementById(`cell-${i}`).textContent =
            currentGame.getCell(i);
    }

    document.getElementById("tic-message").textContent = result;
}

function restartTic() {
    currentGame.newRound();
    showTicTacToe();
}

// WORD SCRAMBLE

function showScramble() {
    gameContent.innerHTML = `
        <h2>Word Scramble</h2>
        <p>Rearrange the letters to find the correct word.</p>

        <h1 id="scrambled-word"></h1>

        <button onclick="showHint()">Show Hint</button>
        <p id="scramble-hint"></p>

        <input id="scramble-input" placeholder="Your answer">
        <button onclick="submitScramble()">Submit</button>

        <p id="scramble-message"></p>
        <p>Score: <span id="scramble-score">0</span></p>

        <button onclick="restartScramble()">Play Again</button>
    `;

    document.getElementById("scrambled-word").textContent =
        currentGame.getScrambledWord();
}

function showHint() {
    document.getElementById("scramble-hint").textContent =
        currentGame.getHint();
}

function submitScramble() {
    const input = document.getElementById("scramble-input");
    const result = currentGame.submitAnswer(input.value);

    document.getElementById("scramble-message").textContent = result;
    document.getElementById("scramble-score").textContent =
        currentGame.getScore();

    input.value = "";
}

function restartScramble() {
    currentGame.newRound();
    showScramble();
}

// NUMBER GUESS

function showNumberGuess() {
    gameContent.innerHTML = `
        <h2>Number Guess</h2>
        <p>Guess the secret number between 1 and 100.</p>

        <input id="number-input" type="number" min="1" max="100"
            placeholder="Enter a number">

        <button onclick="submitNumber()">Guess</button>

        <p id="number-message"></p>
        <p>Attempts: <span id="number-attempts">0</span></p>

        <button onclick="restartNumber()">Play Again</button>
    `;
}

function submitNumber() {
    const input = document.getElementById("number-input");
    const result = currentGame.makeGuess(Number(input.value));

    document.getElementById("number-message").textContent = result;
    document.getElementById("number-attempts").textContent =
        currentGame.getAttempts();

    input.value = "";
}

function restartNumber() {
    currentGame.newRound();
    showNumberGuess();
}

// MAKE FUNCTIONS AVAILABLE TO HTML

Object.assign(window, {
    openGame,
    goHome,
    addWordLetter,
    removeWordLetter,
    submitWordGuess,
    restartWord,
    guessHangmanLetter,
    restartHangman,
    makeTicMove,
    restartTic,
    showHint,
    submitScramble,
    restartScramble,
    submitNumber,
    restartNumber
});
