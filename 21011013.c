#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h> 
#include <windows.h>

#define MAX_NAME_LEN 50
#define MAX_PW_LEN 20
#define MAX_SCORES 5

typedef struct {
    char firstname[MAX_NAME_LEN];
    char lastname[MAX_NAME_LEN];
    char username[MAX_NAME_LEN];
    char password[MAX_PW_LEN];
    int highScores[MAX_SCORES]; // Kullan�c�n�n en y�ksek 5 skoru
} User;

typedef struct {
    User *users; // Dinamik olarak ayr�lan kullan�c�lar i�in pointer
    int userCount;
    int capacity; // Maksimum kullan�c� kapasitesi
    User *activeUser; // Aktif kullan�c�
} AppState;

typedef struct {
    char **map;
    int rows;
    int cols;
} GameMap;

typedef struct {
    int player_x;  				// Oyuncunun x koordinat�
    int player_y;  				// Oyuncunun y koordinat�
    int proton_count;   		// P Toplanan proton say�s�
    int electron_count; 		// e Toplanan elektron say�s�
    int antiproton_count;   	// p Toplanan kar��t proton say�s�
    int positron_count; 		// E Toplanan kar��t elektron say�s�
    int antimatter_count; 		// �retilen kar��t madde say�s�
    int game_over; 				// Oyunun bitip bitmedi�ini kontrol etmek i�in
    int timer;
} GameState;

/* FONKS�YONLAR */
void registerUser(AppState *appState);
void loginUser(AppState *appState);
void displayMenu(AppState *appState);
int loadUsers(AppState *appState);
int saveUsers(const AppState *appState);
void freeAppState(AppState *appState);
void initializeAppState(AppState *appState);
int findUserByUsername(const AppState *appState, const char *username);
void displayGameMenu(AppState *appState);
int loadMap(GameMap *gameMap, const char *filename);
void printMap(const GameMap *gameMap);
void freeMap(GameMap *gameMap);
int startGame(AppState *appState);
void playGame(GameMap *gameMap, GameState *gameState, User *activeUser,AppState *appState);
int movePlayer(GameMap *gameMap, GameState *gameState, int dx, int dy);
void collectParticles(GameMap *gameMap,GameState *gameState, User *activeUser, AppState *appState)  ;
void calculateAntimatterAndScore(GameState *gameState, User *activeUser, AppState *appState);
void updateUserHighScores(User *activeUser, int newScore);
void displayUserHighScores(User *activeUser);
void displayAllUserHighScores(const AppState *appState);
void autoplayGame(GameMap *gameMap, GameState *gameState, User *activeUser, AppState *appState);


int main() {
    AppState appState;
    initializeAppState(&appState);

    if (loadUsers(&appState) != 0) {
        fprintf(stderr, "Error loading user data.\n");
        return 1;
    }

    displayMenu(&appState);
	freeAppState(&appState);
    return 0;
}

// Yeni kullan�c�y� kaydeden fonksiyon
void registerUser(AppState *appState) {
    User newUser;
    printf("Enter first name: ");
    scanf("%49s", newUser.firstname);
    printf("Enter last name: ");
    scanf("%49s", newUser.lastname);
    printf("Choose a username: ");
    scanf("%49s", newUser.username);
    printf("Choose a password: ");
    scanf("%19s", newUser.password);
    int i;
    for (i = 0; i < MAX_SCORES; i++) {
        newUser.highScores[i] = 0; // Yeni kaydolan kullan�c�n�n skorlar�n� 0 ile ba�lat�lmas�
}
        

    if (findUserByUsername(appState, newUser.username) != -1) {
        printf("This username is already taken.\n");
        return;
    }

    // Dinamik
    if (appState->userCount >= appState->capacity) {
        int newCapacity = appState->capacity * 2;
        User *newArray = realloc(appState->users, newCapacity * sizeof(User));
        if (newArray == NULL) {
            fprintf(stderr, "Unable to allocate more memory.\n");
            return;
        }
        appState->users = newArray;
        appState->capacity = newCapacity;
    }

    // Yeni kullan�c�y� ekleme
    appState->users[appState->userCount++] = newUser;
    printf("User registered successfully.\n");
    saveUsers(appState); // Kullan�c�y� kaydeder
}

// Username ve Password ile kullan�c� giri�i
void loginUser(AppState *appState) {
    char username[MAX_NAME_LEN], password[MAX_PW_LEN];
    printf("Username: ");
    scanf("%49s", username);
    printf("Password: ");
    scanf("%19s", password);

    int userIndex = findUserByUsername(appState, username);
    if (userIndex == -1 || strcmp(appState->users[userIndex].password, password) != 0) {
        printf("Invalid username or password.\n");
    } else {
        printf("User logged in successfully.\n");
        appState->activeUser = &appState->users[userIndex];
        displayGameMenu(appState);
    }
}

// Kay�t olunan ve giri� yap�lan ana men�
void displayMenu(AppState *appState) {
    int choice;
    do {
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice == 1) {
            registerUser(appState);
        } else if (choice == 2) {
            loginUser(appState);
        } else if (choice == 3) {
            printf("Exiting program.\n");
            saveUsers(appState); // Save before exiting the program
        } else {
            printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 3);
}

// AppState yap�s�n� ba�lat�r, kullan�c� kapasitesini ayarlar ve kullan�c�lar i�in bellek ay�r�r.
void initializeAppState(AppState *appState) {
    appState->capacity = 5; // Ba�lang�� kapasitesi
    appState->userCount = 0;
    appState->users = malloc(appState->capacity * sizeof(User));
    if (appState->users == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
}


void freeAppState(AppState *appState) {
    free(appState->users);
}



int loadUsers(AppState *appState) {
    char fileName[] = "users.bin";
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        // Dosya yoksa, ilk �al��t�rmada bu normal
        return 0;
    }

    User tempUser;

    while (fread(&tempUser, sizeof(User), 1, file) == 1) {
        // Kullan�c� say�s� kapasiteye ula�t���nda kapasiteyi geni�let
        if (appState->userCount >= appState->capacity) {
            int newCapacity = appState->capacity * 2;
            User *newArray = realloc(appState->users, newCapacity * sizeof(User));
            if (newArray == NULL) {
                fprintf(stderr, "Unable to allocate more memory.\n");
                fclose(file);
                return -1;
            }
            appState->users = newArray;
            appState->capacity = newCapacity;
        }

        // Kullan�c�y� ekleyin
        appState->users[appState->userCount++] = tempUser;
    }

    fclose(file);
    return 0;
}



int saveUsers(const AppState *appState) {
    char fileName[] = "users.bin"; 
    FILE *file = fopen(fileName, "wb"); 
    if (!file) {
        fprintf(stderr, "Unable to open file for writing.\n"); 
        return -1; // Hata durumunda -1 de�eri d�nd�r�l�yor.
    }
	
	// Kullan�c� verilerini dosyaya yazd�rma
    size_t written = fwrite(appState->users, sizeof(User), appState->userCount, file); 
    
    // Yaz�lan veri miktar� beklenenle uyu�muyorsa hata mesaj� yazd�r�l�yor.
    if (written != appState->userCount) {
        fprintf(stderr, "Error writing user data: expected %d, wrote %zu.\n", appState->userCount, written); 
    }

    fclose(file); 
    if (written == appState->userCount) {
        return 0; 
    } else {
        return -1; 
    }
}

// Username'den kullan�c� bulma fonksiyonu
int findUserByUsername(const AppState *appState, const char *username) {
    int i;
    for (i = 0; i < appState->userCount; ++i) {
        if (strcmp(appState->users[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

// Login olduktan sonra oyun men�s�
void displayGameMenu(AppState *appState) {
    int choice;
    do {
        printf("1. Display Your Top Scores\n");
        printf("2. Display Global Top Scores\n");
        printf("3. Game Rules and Instructions\n");
        printf("4. Start Game\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice == 1) {
        displayUserHighScores(appState->activeUser);   
        } else if (choice == 2) {
        displayAllUserHighScores(appState);
        } else if (choice == 3) {
    	printf("\nGame Rules:\n");
    	printf("1. '0' represents empty spaces you can move through, and '1' indicates walls that cannot be passed.\n");
		printf("2. Collect anti-protons ('p') and positrons ('E') to create anti-hydrogen.\n");
		printf("3. Avoid protons ('P') and electrons ('e'), as they can destroy your anti-protons and positrons.\n");
		printf("4. Black holes ('K') will end the game immediately.\n");
		printf("5. Reach the exit ('C') to complete the game.\n");
		printf("6. Act quickly, as the game features a dynamic timer. The timer starts at 10 seconds in 6x10 maps and at 20 seconds in 9x15 maps.\n");
		printf("7. Each anti-proton ('p') and positron ('E') adds 10 seconds to your timer, while each proton ('P') and electron ('e') decreases 5 seconds from your timer.\n");
		printf("8. If you reach the exit ('C'), your points will be calculated.\n");
		printf("9. You will earn 100 points for each unit of anti-hydrogen created and 10 points for each anti-particle collected, plus bonus points for remaining time.\n");
		printf("10. You can choose to watch AI play the game by selecting autoplay mode when you start the game.\n");
		
		
		printf("\nGame Instructions:\n");
		printf("Use the arrow keys to move your character on the map.\n");
		printf("Collect anti-particles to increase your score.\n");
		printf("Aim to reach the exit with the highest score possible. Be quick to maximize your time bonus.\n");
		printf("\nPress any key to return to the main menu...\n");
		getch();
		
        } else if (choice == 4) {
        int mode = startGame(appState);
        if (mode == 1) {
        printf("Manual game mode selected.\n");
        } else {
        printf("Autoplay game mode selected.\n");
        }
        } else if (choice == 5) {
        	printf("Returning to main menu.\n");
        } else   
			printf("Invalid choice. Please try again.\n");
        }
     while (choice != 5);
}

// Dinamik olarak harita okuyan fonksiyon 
int loadMap(GameMap *gameMap, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Unable to open file '%s' for reading.\n", filename);
        return 0;
    }

    char line[1000];
    int rows = 0, maxCols = 0;

    // �lk okumada sat�r ve s�tun say�lar�n� hesapla
    while (fgets(line, sizeof(line), file)) {
        // Sat�r sonu karakterini kald�r
        size_t length = strlen(line);
        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
            length--;
        }
        
        if (length > maxCols) {
            maxCols = length; // En uzun sat�r� bul
        }
        rows++;
    }

    // Harita i�in bellek ay�r
    gameMap->rows = rows;
    gameMap->cols = maxCols;
    gameMap->map = (char **)malloc(rows * sizeof(char *));
    if (!gameMap->map) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(file);
        return 0;
    }

    // Dosyan�n ba��na geri d�n ve haritay� oku
    fseek(file, 0, SEEK_SET);
    int row = 0;
    while (fgets(line, sizeof(line), file) && row < rows) {
        // Her sat�r i�in bellek ay�r
        gameMap->map[row] = (char *)malloc((maxCols + 1) * sizeof(char));
        if (!gameMap->map[row]) {
            fprintf(stderr, "Memory allocation failed for row %d.\n", row);
            fclose(file);
            return 0;
        }
        // Sat�r� harita yap�s�na kopyala
        strncpy(gameMap->map[row], line, maxCols);
        gameMap->map[row][maxCols] = '\0'; // Sat�r sonunu i�aretle
        row++;
    }

    fclose(file);
    return 1;
}

// Harita Belle�ini bo�altma
void freeMap(GameMap *gameMap) {
    int i;
    for (i = 0; i < gameMap->rows; ++i) {
        free(gameMap->map[i]);
    }
    free(gameMap->map);
}

// Harita yazd�rma
void printMap(const GameMap *gameMap) {
    int row, col;
    for (row = 0; row < gameMap->rows; ++row) {
        for (col = 0; col < gameMap->cols; ++col) {
            printf("%c ", gameMap->map[row][col]);
        }
        printf("\n");
    }
}


// int tipine d�n��t�rd�k autoplay/manuel se�imi i�in
int startGame(AppState *appState) {
    GameMap gameMap;
    GameState gameState;
    int mapChoice, gameMode;
    int i, j;

    printf("You can add your own maps by saving them as UTF-8 encoded .txt files.\n");
    printf("Please name them in the format: map_(number between 5 and 10).txt\n");
    
    int mapLoaded = 0; // Haritan�n ba�ar�yla y�klenip y�klenmedi�ini takip etmek i�in
    while (!mapLoaded) {
        printf("Select a map from game directory (1-4) or your own (5-10):\n");
        scanf("%d", &mapChoice);

        if (mapChoice < 1 || mapChoice > 10) {
            printf("Invalid map selection. Please choose a map numbered between 1 and 4. For your custom maps, select a number between 5 and 10.\n");
        } else {
            char filename[20];
            sprintf(filename, "map_%d.txt", mapChoice); // Haritan�n ismi belirleniyor
            mapLoaded = loadMap(&gameMap, filename);
            if (!mapLoaded) {
                printf("Error loading map. Please try again.\n");
            }
        }
    }

    // GameState struct yap�s�n� ayarlama
    char filename[20];
    sprintf(filename, "map_%d.txt", mapChoice); // Haritan�n ismi belirleniyor
    loadMap(&gameMap, filename); // Harita y�kleniyor
    gameState.player_x = 0;
    gameState.player_y = 0;
    gameState.proton_count = 0;
    gameState.electron_count = 0;
    gameState.antiproton_count = 0;
    gameState.positron_count = 0;
    gameState.antimatter_count = 0;
    gameState.game_over = 0;
    	

    // Oyuncunun ba�lang�� noktas�n� ('G') bul ve konumunu ayarla
    	int found = 0; // G noktas�n�n bulunup bulunmad���n� kontrol etmek i�in bayrak
		for (i = 0; i < gameMap.rows && !found; ++i) {
		    for (j = 0; j < gameMap.cols && !found; ++j) {
		        if (gameMap.map[i][j] == 'G') {
		            gameState.player_x = j;
		            gameState.player_y = i;
		            gameMap.map[i][j] = '0';  // Giri� noktas�n� temizle
		            found = 1; 
		        }
		    }
		}

    // Manuel ya da Autoplay se�imi
    printf("Choose game mode: 1 for Manual, 0 for Autoplay:\n");
    scanf("%d", &gameMode);
    
    // 9x15 Haritalarda Timer 20sn 6x10 Haritalarda Timer 10sn
    if (gameMode == 1) {
    	if (mapChoice == 3 || mapChoice == 4){
    	gameState.timer = 20;	
	}
		else {
		gameState.timer = 10;	
	}
        playGame(&gameMap, &gameState, appState->activeUser, appState);
    
	// AI i�in Timer 9x15 Haritalarda 50sn 6x10 Haritalarda Timer 20sn    
    } else {
    	if (mapChoice == 3 || mapChoice == 4){
    	gameState.timer = 50;	
	}
		else {
		gameState.timer = 20;	
	}
        autoplayGame(&gameMap, &gameState, appState->activeUser, appState);
    }

    freeMap(&gameMap); 
    return gameMode;
}


int movePlayer(GameMap *gameMap, GameState *gameState, int dx, int dy) {
    int new_x = gameState->player_x + dx;
    int new_y = gameState->player_y + dy;
    
    // Oyun alan� ve '1' kontrol�
    if (new_x >= 0 && new_x < gameMap->cols && new_y >= 0 && new_y < gameMap->rows &&
        gameMap->map[new_y][new_x] != '1') {
        gameState->player_x = new_x;
        gameState->player_y = new_y;
        return 1; // Ba�ar�l�
    }
    return 0; 
}




void playGame(GameMap *gameMap, GameState *gameState, User *activeUser, AppState *appState) {
    int input;

    do {
        system("cls"); // Ekran� temizle

        // Oyun durumunu burada g�ster
        gameMap->map[gameState->player_y][gameState->player_x] = 'X';
        printMap(gameMap);
        gameMap->map[gameState->player_y][gameState->player_x] = '0';

        // Par�ac�k say�lar�n� ve zamanlay�c�y� burada g�ster
        printf("Protons (P+): %d\n", gameState->proton_count);
        printf("Electrons (e-): %d\n", gameState->electron_count);
        printf("Antiprotons (P-): %d\n", gameState->antiproton_count);
        printf("Positrons (e+): %d\n", gameState->positron_count);
        printf("Time left: %d\n", gameState->timer); // Timer'� g�ster
        
        // validMove bayra�� ile duvara ve oyun s�n�rlar�na �arparsa zaman eklenmiyor
        if (_kbhit()) {
            input = _getch(); // Giri�i al
            if (input == 0 || input == 224) { // Y�n tu�lar� i�in
                input = _getch(); // Ger�ek tu� kodunu al
                int moved = 0;
                switch (input) {
                    case 72: moved = movePlayer(gameMap, gameState, 0, -1); break; // Yukar�
                    case 80: moved = movePlayer(gameMap, gameState, 0, 1); break; // A�a��
                    case 75: moved = movePlayer(gameMap, gameState, -2, 0); break; // Sol 2 ad�m .txtdeki bo�luktan dolay�
                    case 77: moved = movePlayer(gameMap, gameState, 2, 0); break; // Sa� 2 ad�m .txtdeki bo�luktan dolay�
                }
                if (moved) {
                    collectParticles(gameMap, gameState, activeUser, appState);
                }
        
            }
        } else {
            // E�er tu�a bas�lmad�ysa, timer� azalt ve beklemeye ge�
            if (gameState->timer > 0) {
                sleep(1); 
                gameState->timer--;
            } else {
                // E�er timer s�f�ra ula��rsa
                printf("You ran out of time!\n");
                gameState->game_over = 1;
            }
        }
    } while (input != 27 && !gameState->game_over); // ESC tu�una bas�ld���nda veya oyun bitti�inde ��k
}

// Par�ac�k kontrolleri yap�lmas�
void collectParticles(GameMap *gameMap,GameState *gameState, User *activeUser, AppState *appState)  {
    char particle = gameMap->map[gameState->player_y][gameState->player_x];
    switch (particle) {
    	
    	// antiproton ve pozitronlar timere 10 sn ekliyor proton ve elektronlar timerdan 5 sn siliyor
        case 'P': gameState->proton_count++; gameState->timer = gameState->timer + -5; break;
        case 'e': gameState->electron_count++; gameState->timer = gameState->timer + -5; break;
        case 'p': gameState->antiproton_count++; gameState->timer = gameState->timer + 10; break;
        case 'E': gameState->positron_count++; gameState->timer = gameState->timer + 10; break;
        case 'K': 
            gameState->game_over = 1;
            printf("Game Over! Fallen into a black hole.\n");
            break;
        case 'C': 
        	gameState->game_over = 1;
            printf("Reached the exit!\n");
            calculateAntimatterAndScore(gameState,activeUser,appState);
            break;
    }
}

// Kar��t madde say�s�n� ve skoru hesaplayan fonksiyon
void calculateAntimatterAndScore(GameState *gameState, User *activeUser, AppState *appState) {
	
    // Z�t par�ac�klar birbirini yok eder
    while (gameState->proton_count > 0 && gameState->antiproton_count > 0) {
        gameState->proton_count--;
        gameState->antiproton_count--;
    }
    while (gameState->electron_count > 0 && gameState->positron_count > 0) {
        gameState->electron_count--;
        gameState->positron_count--;
    }

    // Kar��t madde say�s�n� hesaplama
    while (gameState->antiproton_count > 0 && gameState->positron_count > 0) {
        gameState->antiproton_count--;
        gameState->positron_count--;
        gameState->antimatter_count++;
    }

    // Skoru hesapla
    int score = (gameState->antiproton_count + gameState->positron_count) * 10 + gameState->antimatter_count * 100 + gameState->timer;

    // Ekrana sonu�lar� yazd�r
    printf("Antimatter produced: %d\n", gameState->antimatter_count);
    printf("Score: %d\n", score);
	updateUserHighScores(activeUser, score);
	saveUsers(appState);
    
}

// Kullan�c�n�n en y�ksek 5 skorunun hesaplanmas�
void updateUserHighScores(User *activeUser, int newScore) {
    int i, j;  
    int scoreInserted = 0; // Yeni skorun eklenip eklenmedi�ini belirten bayrak

    for (i = 0; i < MAX_SCORES && !scoreInserted; i++) {
        // Yeni skorun, mevcut y�ksek skordan daha y�ksek olup olmad���n� kontrol et
        if (newScore > activeUser->highScores[i]) {
            // Daha d���k skorlar� a�a�� kayd�r
            for (j = MAX_SCORES - 1; j > i; j--) {
                activeUser->highScores[j] = activeUser->highScores[j - 1];
            }
            
            activeUser->highScores[i] = newScore;
            scoreInserted = 1; 
        }
    }
}

// Kullan�c�n�n en y�ksek 5 skorunun bast�r�lmas�
void displayUserHighScores(User *activeUser) {
	int i;
    printf("Top Scores for %s:\n", activeUser->username);
    for (i = 0; i < MAX_SCORES; i++) {
        if (activeUser->highScores[i] >= 0) { 
            printf("%d. %d\n", i + 1, activeUser->highScores[i]);
        }
    }
}

// Genel en y�ksek 5 skorun hesaplanmas� s�ralanmas� ve bast�r�lmas�
void displayAllUserHighScores(const AppState *appState) {
    int i, j, k;

    // T�m skorlar� ve kullan�c� adlar�n� tek bir diziye topla
    int totalScores[MAX_SCORES * appState->userCount];
    char *usernames[MAX_SCORES * appState->userCount];

    int index = 0;
    for (i = 0; i < appState->userCount; i++) {
        for (j = 0; j < MAX_SCORES; j++) {
            if (appState->users[i].highScores[j] > 0) {
                totalScores[index] = appState->users[i].highScores[j];
                usernames[index] = appState->users[i].username;
                index++;
            }
        }
    }

    // Insertion sort algoritmas� ile skorlar� s�rala
    for (i = 1; i < index; i++) {
        int currentScore = totalScores[i];
        char *currentUsername = usernames[i];
        j = i - 1;
		
    
        while (j >= 0 && totalScores[j] < currentScore) {
        totalScores[j + 1] = totalScores[j];
        usernames[j + 1] = usernames[j];
        j--;
        }
        totalScores[j + 1] = currentScore;
        usernames[j + 1] = currentUsername;
    }

    // S�ral� skorlar� ve kullan�c� adlar�n� ekrana bas
    printf("Top scores from all users:\n");
    
    // 5'ten az oyun oynanm��sa index kadar 5'ten fazla ise makro kadar bast�r
    for (k = 0; k < MAX_SCORES && k < index; k++) {
        printf("%d. %s - %d\n", k + 1, usernames[k], totalScores[k]);
    }
}



void autoplayGame(GameMap *gameMap, GameState *gameState, User *activeUser, AppState *appState) {
	
    // AI'�n bir y�ne gittikten sonra tekrar geri d�nmemesi i�in bayrak
    int lastMove = -1; // -1 ba�lang�� 0 yukar�, 1 a�a��, 2 sola, 3 sa�a

    // Oyun bitene kadar d�ng�
    while (!gameState->game_over) {
        system("cls"); // Clear the screen

        // Oyun durumunu g�ster
        gameMap->map[gameState->player_y][gameState->player_x] = 'X';
        printMap(gameMap); // Print the map
        gameMap->map[gameState->player_y][gameState->player_x] = '0'; // Reset player position

        // Par�ac�k say�lar�n� ve zamanlay�c�y� yazd�r
        printf("Protons (P+): %d\n", gameState->proton_count);
        printf("Electrons (e-): %d\n", gameState->electron_count);
        printf("Antiprotons (P-): %d\n", gameState->antiproton_count);
        printf("Positrons (e+): %d\n", gameState->positron_count);
        printf("Time left: %d\n", gameState->timer);

        int dx = 0, dy = 0;
        char cellContent;
        int foundTarget = 0;

        // 'p', 'E', 'C' bulma
        // Her bir y�n i�in kontrol yap
        if (!foundTarget && gameState->player_y > 0) {
            cellContent = gameMap->map[gameState->player_y - 1][gameState->player_x];
            if (cellContent == 'p' || cellContent == 'E' || cellContent == 'C') {
                dy = -1; foundTarget = 1; // Yukar�
                lastMove = 0;
            }
        }
        if (!foundTarget && gameState->player_y < gameMap->rows - 1) {
            cellContent = gameMap->map[gameState->player_y + 1][gameState->player_x];
            if (cellContent == 'p' || cellContent == 'E' || cellContent == 'C') {
                dy = 1; foundTarget = 1; // A�a��
                lastMove = 1;
            }
        }
        if (!foundTarget && gameState->player_x > 1) {
            cellContent = gameMap->map[gameState->player_y][gameState->player_x - 2];
            if (cellContent == 'p' || cellContent == 'E' || cellContent == 'C') {
                dx = -2; foundTarget = 1; // Sol
                lastMove = 2;
            }
        }
        if (!foundTarget && gameState->player_x < gameMap->cols - 2) {
            cellContent = gameMap->map[gameState->player_y][gameState->player_x + 2];
            if (cellContent == 'p' || cellContent == 'E' || cellContent == 'C') {
                dx = 2; foundTarget = 1; // Sa�
                lastMove = 3;
            }
        }

        // E�er 'K' yan�ndaysa, farkl� bir y�nde hareket et
        if (!foundTarget) {
            int possibleDirection;
            do {
                possibleDirection = rand() % 4; // Rastgele bir y�n se�
                dx = 0; dy = 0; // Hareketi s�f�rla

                // AI'n�n �nceki hareketinin tersine hareket etmemesini sa�la
                if ((possibleDirection == 0 && lastMove != 1) ||
                    (possibleDirection == 1 && lastMove != 0) ||
                    (possibleDirection == 2 && lastMove != 3) ||
                    (possibleDirection == 3 && lastMove != 2)) {

                    // Se�ilen y�nde duvar veya siyah delik kontrol� yap
                    switch (possibleDirection) {
                        case 0: // Sa�a hareket
                            if (gameState->player_x < gameMap->cols - 2 && gameMap->map[gameState->player_y][gameState->player_x + 2] != 'K' && gameMap->map[gameState->player_y][gameState->player_x + 2] != '1') {
                                dx = 2;
                                if (!(cellContent == 'p' || cellContent == 'E' || cellContent == 'C')) {
				                    lastMove = possibleDirection;
				                } else {
				                    lastMove = -1;
				                }
                            }
                            break;
                        case 1: // Sola hareket
                            if (gameState->player_x > 1 && gameMap->map[gameState->player_y][gameState->player_x - 2] != 'K' && gameMap->map[gameState->player_y][gameState->player_x - 2] != '1') {
                                dx = -2;
                                if (!(cellContent == 'p' || cellContent == 'E' || cellContent == 'C')) {
				                    lastMove = possibleDirection;
				                } else {
				                    lastMove = -1;
				                }
                            }
                            break;
                        case 2: // A�a�� hareket
                            if (gameState->player_y < gameMap->rows - 1 && gameMap->map[gameState->player_y + 1][gameState->player_x] != 'K' && gameMap->map[gameState->player_y + 1][gameState->player_x] != '1') {
                                dy = 1;
                                
                                if (!(cellContent == 'p' || cellContent == 'E' || cellContent == 'C')) {
				                    lastMove = possibleDirection;
				                } else {
				                    lastMove = -1;
				                }
                            }
                            break;
                        case 3: // Yukar� hareket
                            if (gameState->player_y > 0 && gameMap->map[gameState->player_y - 1][gameState->player_x] != 'K' && gameMap->map[gameState->player_y - 1][gameState->player_x] != '1') {
                                dy = -1;
                                if (!(cellContent == 'p' || cellContent == 'E' || cellContent == 'C')) {
				                    lastMove = possibleDirection;
				                } else {
				                    lastMove = -1;
				                }
                            }
                            break;
                    }
                }
            } while (dx == 0 && dy == 0); // Ge�erli bir y�n se�ilene kadar d�ng�
        }

        // Hareketi ger�ekle�tir
        if (dx != 0 || dy != 0) {
            if (movePlayer(gameMap, gameState, dx, dy)) {
                collectParticles(gameMap, gameState, activeUser, appState);
            }
        }

        // Oyun biti� ko�ullar�n� kontrol et
        gameState->timer--;
        if (gameState->timer <= 0) {
            gameState->game_over = 1;
        }

        // G�r�n�rl�k i�in opsiyonel gecikme
        sleep(1);
    }
}





