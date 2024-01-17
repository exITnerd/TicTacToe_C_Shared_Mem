//tworze dwa pliki: Client1 i Client2, buduje przy pomocy gcc i uruchamiam z parametrem, np. game

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define BOARD_SIZE 9
#define PLAYER_X 1
#define PLAYER_O 2

void printBoard(int* board) {
    printf(" %c | %c | %c\n", board[0] ? (board[0] == PLAYER_X ? 'X' : 'O') : '1',
                                  board[1] ? (board[1] == PLAYER_X ? 'X' : 'O') : '2',
                                  board[2] ? (board[2] == PLAYER_X ? 'X' : 'O') : '3');
    printf("---+---+---\n");
    printf(" %c | %c | %c\n", board[3] ? (board[3] == PLAYER_X ? 'X' : 'O') : '4',
                                  board[4] ? (board[4] == PLAYER_X ? 'X' : 'O') : '5',
                                  board[5] ? (board[5] == PLAYER_X ? 'X' : 'O') : '6');
    printf("---+---+---\n");
    printf(" %c | %c | %c\n", board[6] ? (board[6] == PLAYER_X ? 'X' : 'O') : '7',
                                  board[7] ? (board[7] == PLAYER_X ? 'X' : 'O') : '8',
                                  board[8] ? (board[8] == PLAYER_X ? 'X' : 'O') : '9');
}

int checkWin(const int* board, int player) {
    // Check rows, columns, and diagonals
    for (int i = 0; i < 3; ++i) {
        if ((board[i] == player && board[i + 3] == player && board[i + 6] == player) ||  // Check columns
            (board[i * 3] == player && board[i * 3 + 1] == player && board[i * 3 + 2] == player)) {  // Check rows
            return 1;
        }
    }

    // Check diagonals
    if ((board[0] == player && board[4] == player && board[8] == player) ||
        (board[2] == player && board[4] == player && board[6] == player)) {
        return 1;
    }

    return 0;
}

int checkDraw(const int* board) {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        if (board[i] == 0) {
            return 0;  // Board not full
        }
    }
    return 1;  // Board full, but no winner
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Proper usage: %s <file_mapping_name>\n", argv[0]);
        return 1;
    }

    HANDLE hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, argv[1]);

    if (hFileMapping == NULL) {
        // If the shared memory with exact <file_mapping_name> does not exist, create it (you are the cross).
        hFileMapping = CreateFileMapping(
            INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * (BOARD_SIZE + 4), argv[1]);

        if (hFileMapping != NULL) {
            int* pData = (int*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * (BOARD_SIZE + 4));

            if (pData != NULL) {
                printf("Successfully created shared memory: %s\n", argv[1]);
                printf("Waiting for the second player to join...\n");

                while (1) {
                    if (pData[BOARD_SIZE + 1] == 1) {
                        break;
                    }

                    Sleep(1000); // Sleep for 1 second
                }

                printf("Both players have joined. Starting the game...\n");

                printf("You are the X (cross)!\n");
                printBoard(pData);

                pData[BOARD_SIZE] = 1; // currentPlayer (1 for X, 2 for O)
				pData[BOARD_SIZE + 1] = 0; // signal for a new move
				pData[BOARD_SIZE + 2] = 0; // semaphore to control access to the display
				pData[BOARD_SIZE + 3] = 0; // semaphore for PLAYER_X to display message
				pData[BOARD_SIZE + 4] = 0; // semaphore for PLAYER_O to display message


                // Game loop (Player X)
				while (1) {
				    int currentPlayer = pData[BOARD_SIZE];
				    
				    if (currentPlayer == PLAYER_X) {
				        pData[BOARD_SIZE + 3] = 1; // Set semaphore for PLAYER_X
				        printf("Player X, enter your move (1-9): ");
				    } else {
				        while (pData[BOARD_SIZE + 4] == 1) {
				            Sleep(1000); // Sleep for 1 second
				        }
				        pData[BOARD_SIZE + 4] = 0; // Reset semaphore for PLAYER_O
				        printf("Waiting for Player O to move...\n");
				    }
				
				    // Wait for input
				    int move;
				    scanf("%d", &move);
				
				    if (move < 1 || move > 9 || pData[move - 1] != 0) {
				        printf("Invalid move. Try again.\n");
				        continue;
				    }
				
				    pData[move - 1] = currentPlayer;
				
				    // Check for a win or draw
				    if (checkWin(pData, currentPlayer)) {
						printBoard(pData);
				        printf("Player %c wins!\n", currentPlayer == PLAYER_X ? 'X' : 'O');
				        break;
				    } else if (checkDraw(pData)) {
						printBoard(pData);
				        printf("It's a draw!\n");
				        break;
				    }
				
				    // Switch to the other player
				    pData[BOARD_SIZE] = (currentPlayer == PLAYER_X) ? PLAYER_O : PLAYER_X;
				
				    // Allow the other player to display messages
				    pData[BOARD_SIZE + 2] = (pData[BOARD_SIZE] == PLAYER_X) ? 1 : 0;
				
				    // Display the updated board
				    printBoard(pData);
				
				    // Signal that the move has been made
				    pData[BOARD_SIZE + 1] = 1;
				    pData[BOARD_SIZE + 3] = 0; // Reset semaphore for PLAYER_X
				}


                // Clean up
                UnmapViewOfFile(pData);
                CloseHandle(hFileMapping);
            } else {
                printf("Error: Could not map view of the file.\n");
                CloseHandle(hFileMapping);
                return 1;
            }
        } else {
            printf("Error: Could not create shared memory.\n");
            return 1;
        }
    } else {
        int* pData = (int*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int) * (BOARD_SIZE + 4));

        if (pData != NULL) {
            // If shared memory with exact <file_mapping_name> exists, then join it (you are the circle).
            pData[BOARD_SIZE + 1] = 1; // Signal that the other player has joined

            printf("Successfully joined existing shared memory: %s\n", argv[1]);
            printf("Waiting for the game to start...\n");

            while (1) {
                if (pData[BOARD_SIZE + 1] == 0) {
                    break;
                }

                Sleep(1000); // Sleep for 1 second
            }

            printf("Both players have joined. Starting the TicTacToe game...\n");

            printf("You are the O (circle)!\n");
            printBoard(pData);
			
			// -------------- Do poprawy --------------
            // Game loop (Player O)
			while (1) {
			    int currentPlayer = pData[BOARD_SIZE];
			    
			    if (currentPlayer == PLAYER_O) {
			        pData[BOARD_SIZE + 4] = 1; // Set semaphore for PLAYER_O
			        printf("Player O, enter your move (1-9): ");
			    } else {
			        while (pData[BOARD_SIZE + 3] == 1) {
			            Sleep(1000); // Sleep for 1 second
			        }
			        pData[BOARD_SIZE + 3] = 0; // Reset semaphore for PLAYER_X
			        printf("Waiting for Player X to move...\n");
			    }

			
			    // Wait for input
			    int move;
			    scanf("%d", &move);
			
			    if (move < 1 || move > 9 || pData[move - 1] != 0) {
			        printf("Invalid move. Try again.\n");
			        continue;
			    }
			
			    pData[move - 1] = currentPlayer;
			
			    // Check for a win or draw
			    if (checkWin(pData, currentPlayer)) {
					printBoard(pData);
			        printf("Player %c wins!\n", currentPlayer == PLAYER_O ? 'X' : 'O');
			        break;
			    } else if (checkDraw(pData)) {
					printBoard(pData);
			        printf("It's a draw!\n");
			        break;
			    }
			
			    // Switch to the other player
			    pData[BOARD_SIZE] = (currentPlayer == PLAYER_X) ? PLAYER_O : PLAYER_X;
			
			    // Allow the other player to display messages
			    pData[BOARD_SIZE + 2] = (pData[BOARD_SIZE] == PLAYER_X) ? 1 : 0;
			
			    // Display the updated board
			    printBoard(pData);
			
			    // Signal that the move has been made
			    pData[BOARD_SIZE + 1] = 1;
			    pData[BOARD_SIZE + 3] = 0; // Reset semaphore for PLAYER_O
			}
			// -------------- Do poprawy --------------

            // Clean up
            UnmapViewOfFile(pData);
            CloseHandle(hFileMapping);
        } else {
            printf("Error: Could not map view of the file.\n");
            CloseHandle(hFileMapping);
            return 1;
        }
    }

    return 0;
}
