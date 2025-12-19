#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define MAX_Q 10
#define ASK_Q 5
#define NAME_MAX 50
#define LINE_MAX 256
#define RANK_FILE "rankings.txt"

/* ==========================
   1) Cau truc ngan hang cau hoi
   ========================== */
typedef struct {
    char question[200];
    char A[120];
    char B[120];
    char C[120];
    char D[120];
    char correct; /* 'A'..'D' */
} Question;

/* ==========================
   2) Tien ich xu ly chuoi / nhap lieu an toan
   ========================== */

/* Doc 1 dong an toan (fgets), loai bo '\n' o cuoi neu co */
static int readLine(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;  // EOF / error
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return 1;
}

/* Trim khoang trang dau/cuoi */
static void trim(char *s) {
    int start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;

    int end = (int)strlen(s) - 1;
    while (end >= start && isspace((unsigned char)s[end])) end--;

    int i, j = 0;
    for (i = start; i <= end; i++) s[j++] = s[i];
    s[j] = '\0';
}

/* Kiem tra chuoi rong sau trim */
static int isEmptyStr(const char *s) {
    if (!s) return 1;
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

/* Nhap chuoi khong duoc rong */
static void inputNonEmpty(const char *prompt, char *out, int outSize) {
    char line[LINE_MAX];
    while (1) {
        printf("%s", prompt);
        if (!readLine(line, sizeof(line))) {
            printf("\n[LOI] Khong doc duoc input (EOF). Thu lai!\n");
            continue;
        }
        trim(line);
        if (isEmptyStr(line)) {
            printf("[LOI] Khong duoc de trong. Vui long nhap lai!\n");
            continue;
        }
        strncpy(out, line, outSize - 1);
        out[outSize - 1] = '\0';
        return;
    }
}

/* Nhap 1 ky tu lua chon trong tap (vd: A/B/C/D) - khong duoc rong */
static char inputChoiceInSet(const char *prompt, const char *validSet) {
    char line[LINE_MAX];
    while (1) {
        printf("%s", prompt);
        if (!readLine(line, sizeof(line))) {
            printf("\n[LOI] Khong doc duoc input (EOF). Thu lai!\n");
            continue;
        }
        trim(line);
        if (isEmptyStr(line)) {
            printf("[LOI] Khong duoc de trong. Nhap lai!\n");
            continue;
        }
        /* Chi lay ky tu dau tien */
        char c = (char)toupper((unsigned char)line[0]);

        /* Kiem tra co nam trong validSet khong */
        int ok = 0;
        for (int i = 0; validSet[i]; i++) {
            if (c == validSet[i]) { ok = 1; break; }
        }

        if (!ok) {
            printf("[LOI] Chi duoc nhap: %s. Vui long nhap lai!\n", validSet);
            continue;
        }

        return c;
    }
}

/* Nhap so nguyen trong khoang [min..max] */
static int inputIntRange(const char *prompt, int min, int max) {
    char line[LINE_MAX];
    while (1) {
        printf("%s", prompt);
        if (!readLine(line, sizeof(line))) {
            printf("\n[LOI] Khong doc duoc input (EOF). Thu lai!\n");
            continue;
        }
        trim(line);
        if (isEmptyStr(line)) {
            printf("[LOI] Khong duoc de trong. Nhap lai!\n");
            continue;
        }

        char *endptr;
        long val = strtol(line, &endptr, 10);

        /* endptr phai dung cuoi chuoi => input chi la so */
        if (*endptr != '\0') {
            printf("[LOI] Chi duoc nhap so nguyen. Nhap lai!\n");
            continue;
        }
        if (val < min || val > max) {
            printf("[LOI] Vui long nhap trong khoang %d..%d!\n", min, max);
            continue;
        }
        return (int)val;
    }
}

/* ==========================
   3) Random: tron chi so cau hoi (Fisher-Yates)
   ========================== */
static void shuffleIntArray(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* ==========================
   4) Xep hang: luu + doc file
   Format moi dong:
   name|score|unix_time
   ========================== */

typedef struct {
    char name[NAME_MAX];
    int score;
    long t;
} RankItem;

/* So sanh de sort giam dan theo diem, neu bang diem thi moi hon truoc */
static int cmpRank(const void *a, const void *b) {
    const RankItem *x = (const RankItem*)a;
    const RankItem *y = (const RankItem*)b;
    if (y->score != x->score) return y->score - x->score;
    /* neu bang diem -> thoi gian giam dan (moi nhat truoc) */
    if (y->t > x->t) return 1;
    if (y->t < x->t) return -1;
    return 0;
}

static void saveRanking(const char *name, int score) {
    FILE *f = fopen(RANK_FILE, "a");
    if (!f) {
        printf("[CANH BAO] Khong mo duoc file '%s' de luu xep hang!\n", RANK_FILE);
        return;
    }
    long now = (long)time(NULL);
    /* Luu dang an toan: name khong chua ky tu '|' thi ok (neu co thi van luu nhung doc se kho) */
    fprintf(f, "%s|%d|%ld\n", name, score, now);
    fclose(f);
}

static int loadRankings(RankItem *out, int maxItems) {
    FILE *f = fopen(RANK_FILE, "r");
    if (!f) return 0; /* chua co file => coi nhu 0 */

    char line[LINE_MAX];
    int count = 0;

    while (fgets(line, sizeof(line), f) && count < maxItems) {
        /* bo \n */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        /* tach theo '|' */
        char *p1 = strtok(line, "|");
        char *p2 = strtok(NULL, "|");
        char *p3 = strtok(NULL, "|");

        if (!p1 || !p2 || !p3) continue;

        trim(p1); trim(p2); trim(p3);
        if (isEmptyStr(p1)) continue;

        strncpy(out[count].name, p1, NAME_MAX - 1);
        out[count].name[NAME_MAX - 1] = '\0';

        out[count].score = atoi(p2);
        out[count].t = atol(p3);
        count++;
    }

    fclose(f);
    return count;
}

static void showRankings(void) {
    RankItem list[500];
    int n = loadRankings(list, 500);

    printf("\n========== BANG XEP HANG ==========\n");

    if (n == 0) {
        printf("Chua co du lieu xep hang (hoac chua co file %s).\n", RANK_FILE);
        printf("==================================\n");
        return;
    }

    qsort(list, n, sizeof(RankItem), cmpRank);

    printf("%-5s %-25s %-10s %-20s\n", "STT", "Ten", "Diem", "Thoi gian");
    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        /* Doi unix time -> string */
        time_t tt = (time_t)list[i].t;
        struct tm *tm_info = localtime(&tt);
        char timebuf[32];
        if (tm_info) {
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(timebuf, "N/A");
        }

        printf("%-5d %-25s %-10d %-20s\n", i + 1, list[i].name, list[i].score, timebuf);
    }

    printf("==================================\n");
}

/* ==========================
   5) Choi game
   - Chon ngau nhien 5/10 cau
   - Nguoi choi nhap dap an A-D
   - Dung +1 diem
   ========================== */
static void playGame(Question bank[], int bankSize) {
    char player[NAME_MAX];
    inputNonEmpty("Nhap ho ten nguoi choi: ", player, sizeof(player));

    /* tao mang chi so 0..bankSize-1 va tron */
    int idx[MAX_Q];
    for (int i = 0; i < bankSize; i++) idx[i] = i;
    shuffleIntArray(idx, bankSize);

    int score = 0;

    printf("\n===== BAT DAU QUIZ (%d cau) =====\n", ASK_Q);

    for (int i = 0; i < ASK_Q; i++) {
        Question q = bank[idx[i]];

        printf("\nCau %d: %s\n", i + 1, q.question);
        printf("A. %s\n", q.A);
        printf("B. %s\n", q.B);
        printf("C. %s\n", q.C);
        printf("D. %s\n", q.D);

        char ans = inputChoiceInSet("Nhap dap an (A-D): ", "ABCD");

        if (ans == q.correct) {
            printf("=> DUNG! +1 diem\n");
            score++;
        } else {
            printf("=> SAI! Dap an dung la: %c\n", q.correct);
        }
    }

    printf("\n===== KET THUC =====\n");
    printf("Nguoi choi: %s\n", player);
    printf("Tong diem: %d / %d\n", score, ASK_Q);

    /* luu vao file */
    saveRanking(player, score);
    printf("Da luu ket qua vao file: %s\n", RANK_FILE);
}

/* ==========================
   6) MAIN + MENU
   ========================== */
int main(void) {
    srand((unsigned int)time(NULL));

    /* Ngan hang 10 cau hoi (ban co the sua noi dung tuy y) */
    Question bank[MAX_Q] = {
        {"C ngon ngu lap trinh duoc tao boi ai?",
         "Dennis Ritchie", "Bjarne Stroustrup", "James Gosling", "Guido van Rossum", 'A'},

        {"Kieu du lieu nao dung de luu so nguyen trong C?",
         "float", "int", "double", "char*", 'B'},

        {"Ham nao dung de in ra man hinh trong C?",
         "scanf()", "input()", "print()", "printf()", 'D'},

        {"Toan tu AND logic trong C la gi?",
         "&", "&&", "||", "!", 'B'},

        {"Thu vien nao chua ham strlen()?",
         "<stdio.h>", "<math.h>", "<string.h>", "<stdlib.h>", 'C'},

        {"Vong lap nao se chay it nhat 1 lan?",
         "for", "while", "do-while", "if", 'C'},

        {"Ky tu ket thuc chuoi (string) trong C la gi?",
         "\\n", "\\0", "EOF", "\\t", 'B'},

        {"Ham fgets() dung de lam gi?",
         "Doc 1 ky tu", "Doc 1 dong", "Tinh do dai chuoi", "So sanh chuoi", 'B'},

        {"Lenh nao dung de mo file che do ghi them (append)?",
         "fopen(\"a\")", "fopen(\"r\")", "fopen(\"w\")", "fopen(\"rb\")", 'A'},

        {"Trong C, switch-case phu hop nhat khi:",
         "So sanh nhieu dieu kien bang nhau", "Tinh toan so thuc", "Sap xep mang", "Quan ly bo nho", 'A'}
    };

    while (1) {
        printf("\n==============================\n");
        printf("       QUIZ GAME (C)\n");
        printf("==============================\n");
        printf("1. Choi game\n");
        printf("2. Xem bang xep hang\n");
        printf("3. Thoat\n");

        int choice = inputIntRange("Nhap lua chon (1-3): ", 1, 3);

        if (choice == 1) {
            playGame(bank, MAX_Q);
        } else if (choice == 2) {
            showRankings();
        } else {
            printf("Tam biet!\n");
            break;
        }
    }

    return 0;
}

