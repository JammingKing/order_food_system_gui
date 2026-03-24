#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE
#undef _UNICODE
/*
 * ==========================================================
 * 西邮校园食堂菜品管理系统 (图形化完整版)
 * 架构: MVC (Model-View-Controller) 雏形
 * 渲染引擎: EasyX Graphics Library
 * 数据存储: 本地二进制文件 (Dat) + 内存单链表
 * ==========================================================
 * 注意事项:
 * 1. EasyX 的 graphics.h 依赖 C++ 编译器，必须保存为 .cpp 文件。
 * 2. 界面采用状态机 (State Machine) 进行无阻塞的页面路由分发。
 */

#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <stdbool.h>

 // --- 文件持久化路径宏定义 ---
#define WINDOW_FILE "windows.dat"
#define DISH_FILE "dishes.dat"
#define USER_FILE "users.dat"
#define ORDER_FILE "orders.dat"
#define COMMENT_FILE "comments.dat"

// --- GUI 窗口尺寸宏定义 ---
#define GUI_WIDTH 1100
#define GUI_HEIGHT 720

// --- 角色与状态宏定义 ---
#define ROLE_ADMIN 0
#define ROLE_STUDENT 1

#define ORDER_PENDING 0
#define ORDER_COMPLETED 1
#define ORDER_COMMENTED 2

// --- 状态机：页面路由宏定义 ---
#define PAGE_LOGIN 0
#define PAGE_STUDENT 1
#define PAGE_ADMIN 2
#define PAGE_STUDENT_BROWSE 3     // 学生：浏览菜品页面
#define PAGE_STUDENT_MY_ORDERS 4  // 学生：我的订单页面
#define PAGE_ADMIN_WINDOW 5       // 管理员：窗口管理
#define PAGE_ADMIN_DISH 6         // 管理员：菜品管理
#define PAGE_ADMIN_ORDER 7        // 管理员：订单管理
#define PAGE_ADMIN_STAT 8         // 管理员：统计图表页面

// --- 渲染分页参数 ---
#define ITEMS_PER_PAGE 10         // 普通表格每页显示条数
#define BARS_PER_PAGE 6           // 统计图表每页显示柱子数

// --- 全局状态与分页变量 ---
int gTableCurrentPage = 1, gTableTotalPages = 1;             // 学生菜品浏览分页
int gOrderTableCurrentPage = 1, gOrderTableTotalPages = 1;   // 学生订单分页
int gAdminWindowPage = 1, gAdminWindowTotal = 1;             // 管理员窗口分页
int gAdminDishPage = 1, gAdminDishTotal = 1;                 // 管理员菜品分页
int gAdminOrderPage = 1, gAdminOrderTotal = 1;               // 管理员订单分页
int gAdminStatPage = 1, gAdminStatTotal = 1;                 // 管理员图表分页

// --- 模糊搜索全局变量 ---
char gSearchKeyword[50] = "";     // 存放当前搜索的关键字，为空表示未激活搜索

// --- 查询与过滤全局变量 ---
/*char gSearchKeyword[50] = "";    */    // 模糊搜索关键字
char gComboSearchWindow[20] = "";    // 组合查询：窗口ID
char gComboSearchCategory[20] = "";  // 组合查询：菜品分类

/* ================== 数据模型结构体 (Models) ================== */

typedef struct Window {
	char windowId[20];
	char windowName[50];
} Window;

typedef struct WindowNode {
	Window data;
	struct WindowNode* next;
} WindowNode;

typedef struct Dish {
	char dishId[20];
	char windowId[20];
	char name[50];
	char category[20];
	double price;
	int stock;
	double avgScore;
	int scoreCount;
	int sales;
} Dish;

typedef struct DishNode {
	Dish data;
	struct DishNode* next;
} DishNode;

typedef struct User {
	char username[50];
	char password[50];
	int role;
} User;

typedef struct UserNode {
	User data;
	struct UserNode* next;
} UserNode;

typedef struct Order {
	char orderId[20];
	char username[50];
	char dishId[20];
	char dishName[50];
	int quantity;
	double totalPrice;
	int status;
	char orderTime[30];
} Order;

typedef struct OrderNode {
	Order data;
	struct OrderNode* next;
} OrderNode;

typedef struct Comment {
	char commentId[20];
	char orderId[20];
	char dishId[20];
	char username[50];
	int score;
	char content[200];
	char commentTime[30];
} Comment;

typedef struct CommentNode {
	Comment data;
	struct CommentNode* next;
} CommentNode;

// GUI 按钮结构体
typedef struct Button {
	int left;
	int top;
	int right;
	int bottom;
	const char* text;
} Button;

/* ================== 全局 UI 控件实例 ================== */

static Button gBtnBack = { 20, 20, 100, 60, "返回" };
static Button gBtnPrevPage = { 400, 650, 500, 690, "上一页" };
static Button gBtnNextPage = { 600, 650, 700, 690, "下一页" };
static Button gBtnSubmitComment = { 800, 650, 950, 690, "评价订单" };
static Button gBtnAdd = { 700, 650, 780, 690, "添加" };
static Button gBtnDelete = { 900, 650, 980, 690, "删除" };
static Button gBtnUpdateStatus = { 800, 650, 950, 690, "更新状态" };

// --- 新增的 GUI 实体按钮 ---
// --- 顶部工具栏按钮 (Y坐标 85~125，完美避开大标题) ---
static Button gBtnSearch = { 50,  85, 170, 125, "名称搜索" };
static Button gBtnComboSearch = { 190, 85, 310, 125, "组合查询" };
static Button gBtnSortPriceAsc = { 330, 85, 450, 125, "价格升序" };
static Button gBtnSortPriceDesc = { 470, 85, 590, 125, "价格降序" };
static Button gBtnSortSalesDesc = { 610, 85, 730, 125, "销量降序" };
static Button gBtnClearSearch = { 750, 85, 870, 125, "清空条件" };
// --- 新增的管理员修改和导出报表按钮 ---
static Button gBtnEdit = { 800, 650, 880, 690, "修改" };
static Button gBtnExport = { 800, 20, 980, 60, "导出TXT报表" };

static Button gLoginButtons[] = {
	{140, 500, 330, 560, "学生登录"},
	{365, 500, 555, 560, "管理员登录"},
	{590, 500, 780, 560, "学生注册"},
	{815, 500, 960, 560, "退出程序"}
};

static Button gStudentButtons[] = {
	{90, 220, 360, 290, "浏览菜品"},
	{90, 325, 360, 395, "我要点餐"},
	{90, 430, 360, 500, "我的订单"},
	{90, 535, 360, 605, "注销登录"}
};

static Button gAdminButtons[] = {
	{90, 180, 360, 250, "窗口管理"},
	{90, 280, 360, 350, "菜品管理"},
	{90, 380, 360, 450, "订单管理"},
	{90, 480, 360, 550, "评价与统计"},
	{90, 580, 360, 650, "注销登录"}
};

/* ================== 内存链表头指针与系统状态 ================== */

WindowNode* gWindowHead = NULL;
DishNode* gDishHead = NULL;
UserNode* gUserHead = NULL;
OrderNode* gOrderHead = NULL;
CommentNode* gCommentHead = NULL;

User gCurrentUser;               // 记录当前登录用户
int gCurrentPage = PAGE_LOGIN;   // 当前渲染的页面状态
int gHoverIndex = -1;            // 记录鼠标悬停的按钮索引，用于变色反馈
int gRunning = 1;                // 控制主事件循环的开关

/* ================== 函数声明 (Prototypes) ================== */

// 1. 链表 CRUD 声明
WindowNode* insertWindowNode(WindowNode* head, Window item);
WindowNode* deleteWindowNode(WindowNode* head, const char* windowId);
WindowNode* findWindowNode(WindowNode* head, const char* windowId);
void destroyWindowList(WindowNode* head);

DishNode* insertDishNode(DishNode* head, Dish item);
DishNode* deleteDishNode(DishNode* head, const char* dishId);
DishNode* findDishNode(DishNode* head, const char* dishId);
void destroyDishList(DishNode* head);

UserNode* insertUserNode(UserNode* head, User item);
UserNode* deleteUserNode(UserNode* head, const char* username);
UserNode* findUserNode(UserNode* head, const char* username);
void destroyUserList(UserNode* head);

OrderNode* insertOrderNode(OrderNode* head, Order item);
OrderNode* deleteOrderNode(OrderNode* head, const char* orderId);
OrderNode* findOrderNode(OrderNode* head, const char* orderId);
void destroyOrderList(OrderNode* head);

CommentNode* insertCommentNode(CommentNode* head, Comment item);
CommentNode* deleteCommentNode(CommentNode* head, const char* commentId);
CommentNode* findCommentNode(CommentNode* head, const char* commentId);
void destroyCommentList(CommentNode* head);

// 2. 基础辅助与统计声明
void copyText(char* dest, const char* src, size_t size);
void showGuiInfo(const char* title, const char* message);
int stringEmpty(const char* text);
void getCurrentDateTime(char* buffer, size_t size);
void getCurrentDate(char* buffer, size_t size);
void generateOrderId(char* buffer, size_t size);
void generateCommentId(char* buffer, size_t size);
const char* getOrderStatusText(int status);
int countWindows(void);
int countDishes(void);
int countFilteredDishes(void);
int countUsers(void);
int countOrders(void);
int countComments(void);
const char* getWindowNameById(const char* windowId);
int windowHasDish(const char* windowId);
int dishHasOrderReference(const char* dishId);
int dishHasCommentReference(const char* dishId);
int orderHasComment(const char* orderId);
void recalculateDishScore(const char* dishId);
int getGlobalMaxSales(void);

// 3. 文件读写声明
void saveAllData(void);
void loadAllData(void);
void destroyAllData(void);
void cleanup(void);
void initializeDefaultData(void);

// 4. GUI 业务逻辑弹窗声明
void doGuiAddWindow(void);
void doGuiDeleteWindow(void);
void doGuiAddDish(void);
void doGuiDeleteDish(void);
void doGuiUpdateOrderStatus(void);
void doGuiPlaceOrder(void);
void doGuiSubmitComment(void);
void doStudentLogin(void);
void doAdminLogin(void);
void doStudentRegister(void);
void logoutCurrentUser(void);

// 5. GUI 视图渲染声明 (View)
void drawBackground(void);
void drawPanel(int left, int top, int right, int bottom, COLORREF fillColor);
void drawButton(const Button* btn, int hovered, COLORREF baseColor, COLORREF hoverColor);
void drawTag(int x, int y, const char* text, COLORREF fillColor, COLORREF textColor);
void drawSummaryCard(int left, int top, int right, int bottom, const char* title, const char* value, COLORREF accent);
void drawCenteredText(int left, int top, int right, int bottom, const char* text, int fontSize, COLORREF color, int weight);
void drawCurrentPage(void);
void drawLoginPage(void);
void drawStudentPage(void);
void drawAdminPage(void);
void drawStudentBrowsePage(void);
void drawStudentMyOrdersPage(void);
void drawAdminWindowPage(void);
void drawAdminDishPage(void);
void drawAdminOrderPage(void);
void drawAdminStatPage(void);

// 6. GUI 事件监听与路由声明 (Controller)
int pointInButton(int x, int y, const Button* btn);
void handleLoginPage(const MOUSEMSG* msg);
void handleStudentPage(const MOUSEMSG* msg);
void handleAdminPage(const MOUSEMSG* msg);
void handleStudentBrowsePage(const MOUSEMSG* msg);
void handleStudentMyOrdersPage(const MOUSEMSG* msg);
void handleAdminWindowPage(const MOUSEMSG* msg);
void handleAdminDishPage(const MOUSEMSG* msg);
void handleAdminOrderPage(const MOUSEMSG* msg);
void handleAdminStatPage(const MOUSEMSG* msg);


/* ================== 【第一部分】 基础辅助与统计算法 ================== */

// 安全的字符串复制
void copyText(char* dest, const char* src, size_t size) {
	if (dest == NULL || size == 0) return;
	if (src == NULL) { dest[0] = '\0'; return; }
	strncpy(dest, src, size - 1);
	dest[size - 1] = '\0';
}

// GUI 弹窗提示
void showGuiInfo(const char* title, const char* message) {
	MessageBoxA(GetHWnd(), message, title, MB_OK | MB_ICONINFORMATION);
}

// 检查字符串是否为空或全为空格
int stringEmpty(const char* text) {
	if (text == NULL) return 1;
	while (*text != '\0') {
		if (*text != ' ' && *text != '\t' && *text != '\r' && *text != '\n') return 0;
		text++;
	}
	return 1;
}

// 获取当前精准时间
void getCurrentDateTime(char* buffer, size_t size) {
	time_t now = time(NULL);
	struct tm* localInfo = localtime(&now);
	if (localInfo == NULL) { copyText(buffer, "1970-01-01 00:00:00", size); return; }
	strftime(buffer, size, "%Y-%m-%d %H:%M:%S", localInfo);
}

// 获取当前日期 (用于生成流水号)
void getCurrentDate(char* buffer, size_t size) {
	time_t now = time(NULL);
	struct tm* localInfo = localtime(&now);
	if (localInfo == NULL) { copyText(buffer, "19700101", size); return; }
	strftime(buffer, size, "%Y%m%d", localInfo);
}

// 自动生成订单号: 日期+4位流水号 (如 202603240001)
void generateOrderId(char* buffer, size_t size) {
	char datePart[16];
	int sequence = 1;
	OrderNode* current = gOrderHead;
	getCurrentDate(datePart, sizeof(datePart));
	while (current != NULL) {
		if (strncmp(current->data.orderId, datePart, 8) == 0) {
			int currentSeq = atoi(current->data.orderId + 8);
			if (currentSeq >= sequence) sequence = currentSeq + 1;
		}
		current = current->next;
	}
	snprintf(buffer, size, "%s%04d", datePart, sequence);
}

// 自动生成评价号: C+日期+3位流水号
void generateCommentId(char* buffer, size_t size) {
	char datePart[16];
	int sequence = 1;
	CommentNode* current = gCommentHead;
	getCurrentDate(datePart, sizeof(datePart));
	while (current != NULL) {
		if (strncmp(current->data.commentId, "C", 1) == 0 && strncmp(current->data.commentId + 1, datePart, 8) == 0) {
			int currentSeq = atoi(current->data.commentId + 9);
			if (currentSeq >= sequence) sequence = currentSeq + 1;
		}
		current = current->next;
	}
	snprintf(buffer, size, "C%s%03d", datePart, sequence);
}

const char* getOrderStatusText(int status) {
	if (status == ORDER_PENDING) return "待处理";
	if (status == ORDER_COMPLETED) return "已完成";
	return "已评价";
}

// --- 以下为各类数据统计函数，用于仪表盘和分页计算 ---
int countWindows(void) {
	int count = 0; WindowNode* curr = gWindowHead;
	while (curr) { count++; curr = curr->next; } return count;
}

int countDishes(void) {
	int count = 0; DishNode* curr = gDishHead;
	while (curr) { count++; curr = curr->next; } return count;
}

// 带有模糊搜索过滤的菜品数量统计
int countFilteredDishes(void) {
	int count = 0; DishNode* current = gDishHead;
	while (current != NULL) {
		bool matchName = (gSearchKeyword[0] == '\0' || strstr(current->data.name, gSearchKeyword) != NULL);
		bool matchWindow = (gComboSearchWindow[0] == '\0' || strcmp(current->data.windowId, gComboSearchWindow) == 0);
		bool matchCategory = (gComboSearchCategory[0] == '\0' || strcmp(current->data.category, gComboSearchCategory) == 0);

		if (matchName && matchWindow && matchCategory) count++;
		current = current->next;
	}
	return count;
}

int countUsers(void) {
	int count = 0; UserNode* curr = gUserHead;
	while (curr) { count++; curr = curr->next; } return count;
}

int countOrders(void) {
	int count = 0; OrderNode* curr = gOrderHead;
	while (curr) { count++; curr = curr->next; } return count;
}

int countComments(void) {
	int count = 0; CommentNode* curr = gCommentHead;
	while (curr) { count++; curr = curr->next; } return count;
}

const char* getWindowNameById(const char* windowId) {
	WindowNode* node = findWindowNode(gWindowHead, windowId);
	return node ? node->data.windowName : "(未知窗口)";
}

// 关系校验：删除窗口前确认是否有菜品
int windowHasDish(const char* windowId) {
	DishNode* current = gDishHead;
	while (current) {
		if (strcmp(current->data.windowId, windowId) == 0) return 1;
		current = current->next;
	}
	return 0;
}

int dishHasOrderReference(const char* dishId) {
	OrderNode* current = gOrderHead;
	while (current) {
		if (strcmp(current->data.dishId, dishId) == 0) return 1;
		current = current->next;
	}
	return 0;
}

int dishHasCommentReference(const char* dishId) {
	CommentNode* current = gCommentHead;
	while (current) {
		if (strcmp(current->data.dishId, dishId) == 0) return 1;
		current = current->next;
	}
	return 0;
}

int orderHasComment(const char* orderId) {
	CommentNode* current = gCommentHead;
	while (current) {
		if (strcmp(current->data.orderId, orderId) == 0) return 1;
		current = current->next;
	}
	return 0;
}

// 核心业务：当新评价产生时，动态重算菜品的平均分
void recalculateDishScore(const char* dishId) {
	DishNode* dishNode = findDishNode(gDishHead, dishId);
	if (!dishNode) return;
	int count = 0; int total = 0;
	CommentNode* current = gCommentHead;
	while (current != NULL) {
		if (strcmp(current->data.dishId, dishId) == 0) {
			count++; total += current->data.score;
		}
		current = current->next;
	}
	dishNode->data.scoreCount = count;
	dishNode->data.avgScore = count == 0 ? 0.0 : (double)total / count;
}

// 图表算法：获取全局最大销量，用于确定 Y 轴比例尺归一化
int getGlobalMaxSales(void) {
	int max = 0; DishNode* current = gDishHead;
	while (current != NULL) {
		if (current->data.sales > max) max = current->data.sales;
		current = current->next;
	}
	return max > 0 ? max : 1;
}


/* ================== 【第二部分】 内存单链表操作层 (Model) ================== */
// 注意：以下包含窗口、菜品、用户、订单、评价的增、查、删功能。尾插法。

WindowNode* insertWindowNode(WindowNode* head, Window item) {
	WindowNode* node = (WindowNode*)malloc(sizeof(WindowNode));
	if (!node) return head;
	node->data = item; node->next = NULL;
	if (!head) return node;
	WindowNode* curr = head;
	while (curr->next) curr = curr->next;
	curr->next = node; return head;
}

WindowNode* deleteWindowNode(WindowNode* head, const char* windowId) {
	if (!head) return NULL;
	if (strcmp(head->data.windowId, windowId) == 0) {
		WindowNode* temp = head; head = head->next; free(temp); return head;
	}
	WindowNode* curr = head;
	while (curr->next) {
		if (strcmp(curr->next->data.windowId, windowId) == 0) {
			WindowNode* temp = curr->next; curr->next = temp->next; free(temp); return head;
		}
		curr = curr->next;
	}
	return head;
}

WindowNode* findWindowNode(WindowNode* head, const char* windowId) {
	WindowNode* curr = head;
	while (curr) {
		if (strcmp(curr->data.windowId, windowId) == 0) return curr;
		curr = curr->next;
	}
	return NULL;
}

void destroyWindowList(WindowNode* head) {
	while (head) { WindowNode* temp = head; head = head->next; free(temp); }
}

// --- 菜品节点 ---
DishNode* insertDishNode(DishNode* head, Dish item) {
	DishNode* node = (DishNode*)malloc(sizeof(DishNode));
	if (!node) return head;
	node->data = item; node->next = NULL;
	if (!head) return node;
	DishNode* curr = head;
	while (curr->next) curr = curr->next;
	curr->next = node; return head;
}

DishNode* deleteDishNode(DishNode* head, const char* dishId) {
	if (!head) return NULL;
	if (strcmp(head->data.dishId, dishId) == 0) {
		DishNode* temp = head; head = head->next; free(temp); return head;
	}
	DishNode* curr = head;
	while (curr->next) {
		if (strcmp(curr->next->data.dishId, dishId) == 0) {
			DishNode* temp = curr->next; curr->next = temp->next; free(temp); return head;
		}
		curr = curr->next;
	}
	return head;
}

DishNode* findDishNode(DishNode* head, const char* dishId) {
	DishNode* curr = head;
	while (curr) {
		if (strcmp(curr->data.dishId, dishId) == 0) return curr;
		curr = curr->next;
	}
	return NULL;
}

void destroyDishList(DishNode* head) {
	while (head) { DishNode* temp = head; head = head->next; free(temp); }
}

// --- 用户节点 ---
UserNode* insertUserNode(UserNode* head, User item) {
	UserNode* node = (UserNode*)malloc(sizeof(UserNode));
	if (!node) return head;
	node->data = item; node->next = NULL;
	if (!head) return node;
	UserNode* curr = head;
	while (curr->next) curr = curr->next;
	curr->next = node; return head;
}

UserNode* deleteUserNode(UserNode* head, const char* username) {
	if (!head) return NULL;
	if (strcmp(head->data.username, username) == 0) {
		UserNode* temp = head; head = head->next; free(temp); return head;
	}
	UserNode* curr = head;
	while (curr->next) {
		if (strcmp(curr->next->data.username, username) == 0) {
			UserNode* temp = curr->next; curr->next = temp->next; free(temp); return head;
		}
		curr = curr->next;
	}
	return head;
}

UserNode* findUserNode(UserNode* head, const char* username) {
	UserNode* curr = head;
	while (curr) {
		if (strcmp(curr->data.username, username) == 0) return curr;
		curr = curr->next;
	}
	return NULL;
}

void destroyUserList(UserNode* head) {
	while (head) { UserNode* temp = head; head = head->next; free(temp); }
}

// --- 订单节点 ---
OrderNode* insertOrderNode(OrderNode* head, Order item) {
	OrderNode* node = (OrderNode*)malloc(sizeof(OrderNode));
	if (!node) return head;
	node->data = item; node->next = NULL;
	if (!head) return node;
	OrderNode* curr = head;
	while (curr->next) curr = curr->next;
	curr->next = node; return head;
}

OrderNode* deleteOrderNode(OrderNode* head, const char* orderId) {
	if (!head) return NULL;
	if (strcmp(head->data.orderId, orderId) == 0) {
		OrderNode* temp = head; head = head->next; free(temp); return head;
	}
	OrderNode* curr = head;
	while (curr->next) {
		if (strcmp(curr->next->data.orderId, orderId) == 0) {
			OrderNode* temp = curr->next; curr->next = temp->next; free(temp); return head;
		}
		curr = curr->next;
	}
	return head;
}

OrderNode* findOrderNode(OrderNode* head, const char* orderId) {
	OrderNode* curr = head;
	while (curr) {
		if (strcmp(curr->data.orderId, orderId) == 0) return curr;
		curr = curr->next;
	}
	return NULL;
}

void destroyOrderList(OrderNode* head) {
	while (head) { OrderNode* temp = head; head = head->next; free(temp); }
}

// --- 评价节点 ---
CommentNode* insertCommentNode(CommentNode* head, Comment item) {
	CommentNode* node = (CommentNode*)malloc(sizeof(CommentNode));
	if (!node) return head;
	node->data = item; node->next = NULL;
	if (!head) return node;
	CommentNode* curr = head;
	while (curr->next) curr = curr->next;
	curr->next = node; return head;
}

CommentNode* deleteCommentNode(CommentNode* head, const char* commentId) {
	if (!head) return NULL;
	if (strcmp(head->data.commentId, commentId) == 0) {
		CommentNode* temp = head; head = head->next; free(temp); return head;
	}
	CommentNode* curr = head;
	while (curr->next) {
		if (strcmp(curr->next->data.commentId, commentId) == 0) {
			CommentNode* temp = curr->next; curr->next = temp->next; free(temp); return head;
		}
		curr = curr->next;
	}
	return head;
}

CommentNode* findCommentNode(CommentNode* head, const char* commentId) {
	CommentNode* curr = head;
	while (curr) {
		if (strcmp(curr->data.commentId, commentId) == 0) return curr;
		curr = curr->next;
	}
	return NULL;
}

void destroyCommentList(CommentNode* head) {
	while (head) { CommentNode* temp = head; head = head->next; free(temp); }
}




/* ================== 【第三部分】 本地文件序列化持久层 ================== */

void saveWindowsToFile(void) {
	FILE* fp = fopen(WINDOW_FILE, "wb");
	if (!fp) return;
	WindowNode* curr = gWindowHead;
	while (curr) { fwrite(&curr->data, sizeof(Window), 1, fp); curr = curr->next; }
	fclose(fp);
}
void loadWindowsFromFile(void) {
	FILE* fp = fopen(WINDOW_FILE, "rb");
	if (!fp) return;
	Window item;
	while (fread(&item, sizeof(Window), 1, fp) == 1) gWindowHead = insertWindowNode(gWindowHead, item);
	fclose(fp);
}

void saveDishesToFile(void) {
	FILE* fp = fopen(DISH_FILE, "wb");
	if (!fp) return;
	DishNode* curr = gDishHead;
	while (curr) { fwrite(&curr->data, sizeof(Dish), 1, fp); curr = curr->next; }
	fclose(fp);
}
void loadDishesFromFile(void) {
	FILE* fp = fopen(DISH_FILE, "rb");
	if (!fp) return;
	Dish item;
	while (fread(&item, sizeof(Dish), 1, fp) == 1) gDishHead = insertDishNode(gDishHead, item);
	fclose(fp);
}

void saveUsersToFile(void) {
	FILE* fp = fopen(USER_FILE, "wb");
	if (!fp) return;
	UserNode* curr = gUserHead;
	while (curr) { fwrite(&curr->data, sizeof(User), 1, fp); curr = curr->next; }
	fclose(fp);
}
void loadUsersFromFile(void) {
	FILE* fp = fopen(USER_FILE, "rb");
	if (!fp) return;
	User item;
	while (fread(&item, sizeof(User), 1, fp) == 1) gUserHead = insertUserNode(gUserHead, item);
	fclose(fp);
}

void saveOrdersToFile(void) {
	FILE* fp = fopen(ORDER_FILE, "wb");
	if (!fp) return;
	OrderNode* curr = gOrderHead;
	while (curr) { fwrite(&curr->data, sizeof(Order), 1, fp); curr = curr->next; }
	fclose(fp);
}
void loadOrdersFromFile(void) {
	FILE* fp = fopen(ORDER_FILE, "rb");
	if (!fp) return;
	Order item;
	while (fread(&item, sizeof(Order), 1, fp) == 1) gOrderHead = insertOrderNode(gOrderHead, item);
	fclose(fp);
}

void saveCommentsToFile(void) {
	FILE* fp = fopen(COMMENT_FILE, "wb");
	if (!fp) return;
	CommentNode* curr = gCommentHead;
	while (curr) { fwrite(&curr->data, sizeof(Comment), 1, fp); curr = curr->next; }
	fclose(fp);
}
void loadCommentsFromFile(void) {
	FILE* fp = fopen(COMMENT_FILE, "rb");
	if (!fp) return;
	Comment item;
	while (fread(&item, sizeof(Comment), 1, fp) == 1) gCommentHead = insertCommentNode(gCommentHead, item);
	fclose(fp);
}

void saveAllData(void) {
	saveWindowsToFile(); saveDishesToFile(); saveUsersToFile(); saveOrdersToFile(); saveCommentsToFile();
}

void loadAllData(void) {
	loadWindowsFromFile(); loadDishesFromFile(); loadUsersFromFile(); loadOrdersFromFile(); loadCommentsFromFile();
}

void destroyAllData(void) {
	destroyWindowList(gWindowHead);
	destroyDishList(gDishHead);
	destroyUserList(gUserHead);
	destroyOrderList(gOrderHead);
	destroyCommentList(gCommentHead);

	// 将连等拆开，分别单独赋值为 NULL
	gWindowHead = NULL;
	gDishHead = NULL;
	gUserHead = NULL;
	gOrderHead = NULL;
	gCommentHead = NULL;
}

void cleanup(void) {
	saveAllData(); destroyAllData();
}

// 首次运行环境初始化：注入测试数据
void initializeDefaultData(void) {
	Window defaultWindow1, defaultWindow2;
	Dish defaultDish1, defaultDish2;
	User defaultAdmin;

	if (!findUserNode(gUserHead, "admin")) {
		copyText(defaultAdmin.username, "admin", sizeof(defaultAdmin.username));
		copyText(defaultAdmin.password, "123456", sizeof(defaultAdmin.password));
		defaultAdmin.role = ROLE_ADMIN;
		gUserHead = insertUserNode(gUserHead, defaultAdmin);
	}

	if (!findWindowNode(gWindowHead, "W001")) {
		copyText(defaultWindow1.windowId, "W001", sizeof(defaultWindow1.windowId));
		copyText(defaultWindow1.windowName, "家常菜窗口", sizeof(defaultWindow1.windowName));
		gWindowHead = insertWindowNode(gWindowHead, defaultWindow1);
	}

	if (!findWindowNode(gWindowHead, "W002")) {
		copyText(defaultWindow2.windowId, "W002", sizeof(defaultWindow2.windowId));
		copyText(defaultWindow2.windowName, "面食窗口", sizeof(defaultWindow2.windowName));
		gWindowHead = insertWindowNode(gWindowHead, defaultWindow2);
	}

	if (!findDishNode(gDishHead, "D001")) {
		copyText(defaultDish1.dishId, "D001", sizeof(defaultDish1.dishId));
		copyText(defaultDish1.windowId, "W001", sizeof(defaultDish1.windowId));
		copyText(defaultDish1.name, "番茄炒蛋", sizeof(defaultDish1.name));
		copyText(defaultDish1.category, "家常菜", sizeof(defaultDish1.category));
		defaultDish1.price = 12.0; defaultDish1.stock = 60; defaultDish1.sales = 0;
		defaultDish1.avgScore = 0.0; defaultDish1.scoreCount = 0;
		gDishHead = insertDishNode(gDishHead, defaultDish1);
	}

	if (!findDishNode(gDishHead, "D002")) {
		copyText(defaultDish2.dishId, "D002", sizeof(defaultDish2.dishId));
		copyText(defaultDish2.windowId, "W002", sizeof(defaultDish2.windowId));
		copyText(defaultDish2.name, "牛肉面", sizeof(defaultDish2.name));
		copyText(defaultDish2.category, "面食", sizeof(defaultDish2.category));
		defaultDish2.price = 16.0; defaultDish2.stock = 50; defaultDish2.sales = 0;
		defaultDish2.avgScore = 0.0; defaultDish2.scoreCount = 0;
		gDishHead = insertDishNode(gDishHead, defaultDish2);
	}
	saveAllData();
}


/* ================== 【第四部分】 业务逻辑层：GUI 事件响应函数 (Services) ================== */

// -- 管理员业务：添加/删除窗口 --
void doGuiAddWindow(void) {
	Window item;
	if (!InputBox(item.windowId, 20, "请输入新窗口 ID:", "添加窗口")) return;
	if (stringEmpty(item.windowId) || findWindowNode(gWindowHead, item.windowId) != NULL) {
		showGuiInfo("错误", "窗口 ID 不能为空，且不能与现有 ID 重复。"); return;
	}
	if (!InputBox(item.windowName, 50, "请输入新窗口名称:", "添加窗口")) return;

	gWindowHead = insertWindowNode(gWindowHead, item);
	saveAllData(); showGuiInfo("成功", "窗口添加成功！");
}

void doGuiDeleteWindow(void) {
	char id[20];
	if (!InputBox(id, 20, "请输入要删除的窗口 ID:", "删除窗口")) return;
	if (findWindowNode(gWindowHead, id) == NULL) { showGuiInfo("错误", "未找到该窗口。"); return; }
	if (windowHasDish(id)) { showGuiInfo("错误", "无法删除：该窗口下仍有菜品！"); return; }

	gWindowHead = deleteWindowNode(gWindowHead, id);
	saveAllData(); showGuiInfo("成功", "窗口删除成功！");
}

// -- 管理员业务：添加/删除菜品 --
void doGuiAddDish(void) {
	Dish item; char temp[20];

	if (!InputBox(item.dishId, 20, "1/6 请输入新菜品 ID:", "添加菜品")) return;
	if (findDishNode(gDishHead, item.dishId) != NULL) { showGuiInfo("错误", "菜品 ID 已存在。"); return; }
	if (!InputBox(item.windowId, 20, "2/6 请输入所属窗口 ID:", "添加菜品")) return;
	if (findWindowNode(gWindowHead, item.windowId) == NULL) { showGuiInfo("错误", "该窗口 ID 不存在。"); return; }
	if (!InputBox(item.name, 50, "3/6 请输入菜品名称:", "添加菜品")) return;
	if (!InputBox(item.category, 20, "4/6 请输入分类(如: 家常菜):", "添加菜品")) return;

	if (!InputBox(temp, 20, "5/6 请输入价格(>0):", "添加菜品")) return;
	item.price = atof(temp);
	if (item.price <= 0) { showGuiInfo("错误", "价格必须大于0。"); return; }

	if (!InputBox(temp, 20, "6/6 请输入初始库存:", "添加菜品")) return;
	item.stock = atoi(temp);

	item.avgScore = 0.0; item.scoreCount = 0; item.sales = 0;
	gDishHead = insertDishNode(gDishHead, item);
	saveAllData(); showGuiInfo("成功", "菜品添加成功！");
}

void doGuiDeleteDish(void) {
	char id[20];
	if (!InputBox(id, 20, "请输入要删除的菜品 ID:", "删除菜品")) return;
	if (findDishNode(gDishHead, id) == NULL) { showGuiInfo("错误", "未找到该菜品。"); return; }
	if (dishHasOrderReference(id) || dishHasCommentReference(id)) {
		showGuiInfo("错误", "无法删除：已有订单或评价关联了此菜品。"); return;
	}
	gDishHead = deleteDishNode(gDishHead, id);
	saveAllData(); showGuiInfo("成功", "菜品删除成功！");
}

// -- 管理员业务：更新订单状态 --
void doGuiUpdateOrderStatus(void) {
	char id[20];
	if (!InputBox(id, 20, "请输入要更新的订单 ID:", "更新订单")) return;
	OrderNode* node = findOrderNode(gOrderHead, id);
	if (node == NULL) { showGuiInfo("错误", "未找到该订单。"); return; }

	if (node->data.status == ORDER_PENDING) {
		if (MessageBoxA(GetHWnd(), "确认将该订单标记为已完成吗？", "更新确认", MB_YESNO | MB_ICONQUESTION) == IDYES) {
			node->data.status = ORDER_COMPLETED; saveAllData();
			showGuiInfo("成功", "订单状态已更新为“已完成”。");
		}
	}
	else showGuiInfo("提示", "该订单已完成或已评价，无需手动更新。");
}

// -- 学生业务：图形化点餐下单 --
void doGuiPlaceOrder(void) {
	char dishId[20] = "", quantityStr[20] = "";
	DishNode* dishNode = NULL; Order item;

	if (!InputBox(dishId, 20, "请输入要购买的菜品 ID:", "学生点餐", "", 300, 200, false)) return;
	if (stringEmpty(dishId)) { showGuiInfo("点餐失败", "菜品 ID 不能为空。"); return; }

	dishNode = findDishNode(gDishHead, dishId);
	if (dishNode == NULL) { showGuiInfo("点餐失败", "未找到该菜品，请核对 ID。"); return; }
	if (dishNode->data.stock <= 0) { showGuiInfo("点餐失败", "抱歉，该菜品已售罄。"); return; }

	char prompt[128];
	snprintf(prompt, sizeof(prompt), "菜品：%s\n当前库存：%d\n请输入购买数量:", dishNode->data.name, dishNode->data.stock);
	if (!InputBox(quantityStr, 20, prompt, "输入数量", "1", 300, 200, false)) return;

	item.quantity = atoi(quantityStr);
	if (item.quantity <= 0) { showGuiInfo("点餐失败", "购买数量必须大于 0。"); return; }
	if (item.quantity > dishNode->data.stock) { showGuiInfo("点餐失败", "库存不足！"); return; }

	generateOrderId(item.orderId, sizeof(item.orderId));
	copyText(item.username, gCurrentUser.username, sizeof(item.username));
	copyText(item.dishId, dishNode->data.dishId, sizeof(item.dishId));
	copyText(item.dishName, dishNode->data.name, sizeof(item.dishName));
	item.totalPrice = dishNode->data.price * item.quantity;
	item.status = ORDER_PENDING;
	getCurrentDateTime(item.orderTime, sizeof(item.orderTime));

	dishNode->data.stock -= item.quantity;
	dishNode->data.sales += item.quantity;
	gOrderHead = insertOrderNode(gOrderHead, item);
	saveAllData();

	char successMsg[256];
	snprintf(successMsg, sizeof(successMsg), "下单成功！\n订单号：%s\n总价：%.2f 元", item.orderId, item.totalPrice);
	showGuiInfo("操作成功", successMsg);
}

// -- 学生业务：图形化提交评价 --
void doGuiSubmitComment(void) {
	char orderId[20] = "", scoreStr[10] = "", content[200] = "";
	OrderNode* orderNode = NULL; DishNode* dishNode = NULL; Comment item;

	if (!InputBox(orderId, 20, "请输入要评价的【已完成】订单 ID:", "提交评价")) return;
	orderNode = findOrderNode(gOrderHead, orderId);
	if (!orderNode) { showGuiInfo("评价失败", "未找到该订单。"); return; }
	if (strcmp(orderNode->data.username, gCurrentUser.username) != 0) { showGuiInfo("评价失败", "您只能评价自己的订单。"); return; }
	if (orderNode->data.status != ORDER_COMPLETED) {
		showGuiInfo("评价失败", orderNode->data.status == ORDER_COMMENTED ? "此订单已评价过。" : "此订单尚未完成，无法评价。"); return;
	}
	if (orderHasComment(orderId)) { showGuiInfo("评价失败", "此订单已存在评价记录。"); return; }

	if (!InputBox(scoreStr, 10, "请输入评分 (1-5的整数):", "提交评价", "5")) return;
	int score = atoi(scoreStr);
	if (score < 1 || score > 5) { showGuiInfo("评价失败", "评分必须在 1 到 5 之间。"); return; }
	if (!InputBox(content, 200, "请输入您的文字评价:", "提交评价", "味道很不错！")) return;

	generateCommentId(item.commentId, sizeof(item.commentId));
	copyText(item.orderId, orderNode->data.orderId, sizeof(item.orderId));
	copyText(item.dishId, orderNode->data.dishId, sizeof(item.dishId));
	copyText(item.username, gCurrentUser.username, sizeof(item.username));
	item.score = score;
	copyText(item.content, content, sizeof(item.content));
	getCurrentDateTime(item.commentTime, sizeof(item.commentTime));

	gCommentHead = insertCommentNode(gCommentHead, item);
	orderNode->data.status = ORDER_COMMENTED;
	dishNode = findDishNode(gDishHead, orderNode->data.dishId);
	if (dishNode) recalculateDishScore(dishNode->data.dishId);

	saveAllData(); showGuiInfo("评价成功", "感谢您的评价！");
}

// -- 登录注册模块：控制台密码输入 + GUI状态控制 --
void doStudentLogin(void) {
	char username[50] = "", password[50] = ""; UserNode* node = NULL;
	if (!InputBox(username, 50, "请输入用户名", "学生登录")) return;
	if (stringEmpty(username)) { showGuiInfo("登录失败", "用户名不能为空。"); return; }

	printf("请在控制台输入密码 (输入完成时按回车键继续): ");
	char ch; int i = 0;
	while ((ch = _getch()) != '\r') {
		if (ch == '\b' && i > 0) { printf("\b \b"); i--; }
		else if (i < sizeof(password) - 1) { password[i++] = ch; printf("*"); }
	}
	password[i] = '\0'; printf("\n");

	if (stringEmpty(password)) { showGuiInfo("登录失败", "密码不能为空。"); return; }
	node = findUserNode(gUserHead, username);
	if (node == NULL || node->data.role != ROLE_STUDENT || strcmp(node->data.password, password) != 0) {
		showGuiInfo("登录失败", "学生用户名或密码错误。"); return;
	}
	gCurrentUser = node->data; gCurrentPage = PAGE_STUDENT; gHoverIndex = -1;
	showGuiInfo("登录成功", "欢迎进入学生中心。");
}

void doAdminLogin(void) {
	char username[50] = "", password[50] = ""; UserNode* node = NULL;
	if (!InputBox(username, 50, "请输入用户名", "管理员登录")) return;
	if (stringEmpty(username)) { showGuiInfo("登录失败", "用户名不能为空。"); return; }

	printf("请在控制台输入密码 (输入完成时按回车键继续): ");
	char ch; int i = 0;
	while ((ch = _getch()) != '\r') {
		if (ch == '\b' && i > 0) { printf("\b \b"); i--; }
		else if (i < sizeof(password) - 1) { password[i++] = ch; printf("*"); }
	}
	password[i] = '\0'; printf("\n");

	node = findUserNode(gUserHead, username);
	if (node == NULL || node->data.role != ROLE_ADMIN || strcmp(node->data.password, password) != 0) {
		showGuiInfo("登录失败", "管理员用户名或密码错误。"); return;
	}
	gCurrentUser = node->data; gCurrentPage = PAGE_ADMIN; gHoverIndex = -1;
	showGuiInfo("登录成功", "欢迎进入管理员中心。");
}

void doStudentRegister(void) {
	User item; char confirmPassword[50] = "";
	if (!InputBox(item.username, 50, "请输入新用户名", "学生注册")) return;
	if (strlen(item.username) < 3 || strlen(item.username) > 50) { showGuiInfo("注册失败", "用户名长度需在3-50之间。"); return; }
	if (findUserNode(gUserHead, item.username) != NULL) { showGuiInfo("注册失败", "该用户名已存在。"); return; }

	printf("请在控制台输入注册密码（输入完成时按回车键继续）: ");
	char ch; int i = 0;
	while ((ch = _getch()) != '\r') {
		if (ch == '\b' && i > 0) { printf("\b \b"); i--; }
		else if (i < sizeof(item.password) - 1) { item.password[i++] = ch; printf("*"); }
	}
	item.password[i] = '\0';

	printf("\n请在控制台确认密码（输入完成时按回车键继续）: "); i = 0;
	while ((ch = _getch()) != '\r') {
		if (ch == '\b' && i > 0) { printf("\b \b"); i--; }
		else if (i < sizeof(confirmPassword) - 1) { confirmPassword[i++] = ch; printf("*"); }
	}
	confirmPassword[i] = '\0'; printf("\n");

	if (strcmp(item.password, confirmPassword) != 0) { showGuiInfo("注册失败", "两次密码不一致。"); return; }
	if (strlen(item.password) < 6) { showGuiInfo("注册失败", "密码长度需大于6。"); return; }

	item.role = ROLE_STUDENT;
	gUserHead = insertUserNode(gUserHead, item);
	saveAllData(); showGuiInfo("注册成功", "学生账号创建成功。");
}

void logoutCurrentUser(void) {
	memset(&gCurrentUser, 0, sizeof(gCurrentUser));
	gCurrentPage = PAGE_LOGIN; gHoverIndex = -1;
}

// --- 要求4：数据修改 ---
void doGuiModifyDish(void) {
	char id[20]; char tempStr[50];
	if (!InputBox(id, 20, "请输入要修改的菜品 ID:", "修改菜品")) return;
	DishNode* node = findDishNode(gDishHead, id);
	if (!node) { showGuiInfo("错误", "未找到该菜品！"); return; }

	if (InputBox(tempStr, 50, "请输入新菜品名称(留空则不改):", "修改数据")) {
		if (!stringEmpty(tempStr)) copyText(node->data.name, tempStr, sizeof(node->data.name));
	}
	if (InputBox(tempStr, 20, "请输入新价格(留空则不改):", "修改数据")) {
		if (!stringEmpty(tempStr)) node->data.price = atof(tempStr);
	}
	saveAllData();
	showGuiInfo("成功", "菜品数据修改成功！");
}

// --- 要求9：数据排序 (价格升序) ---
void sortDishesByPriceAsc() {
	if (!gDishHead || !gDishHead->next) return;
	for (DishNode* i = gDishHead; i != NULL; i = i->next) {
		for (DishNode* j = i->next; j != NULL; j = j->next) {
			if (i->data.price > j->data.price) {
				Dish temp = i->data; i->data = j->data; j->data = temp;
			}
		}
	}
}

// --- 要求9：数据排序 (价格降序) ---
void sortDishesByPriceDesc() {
	if (!gDishHead || !gDishHead->next) return;
	for (DishNode* i = gDishHead; i != NULL; i = i->next) {
		for (DishNode* j = i->next; j != NULL; j = j->next) {
			if (i->data.price < j->data.price) {
				Dish temp = i->data; i->data = j->data; j->data = temp;
			}
		}
	}
}

// --- 要求9：数据排序 (销量降序) ---
void sortDishesBySalesDesc() {
	if (!gDishHead || !gDishHead->next) return;
	for (DishNode* i = gDishHead; i != NULL; i = i->next) {
		for (DishNode* j = i->next; j != NULL; j = j->next) {
			if (i->data.sales < j->data.sales) {
				Dish temp = i->data; i->data = j->data; j->data = temp;
			}
		}
	}
}

// --- 要求10：数据报表导出 ---
void doGuiExportReport() {
	FILE* fp = fopen("食堂数据报表.txt", "w");
	if (!fp) { showGuiInfo("错误", "报表创建失败！"); return; }

	fprintf(fp, "================================ 西邮食堂菜品综合报表 ================================\n");
	char genTime[30]; getCurrentDateTime(genTime, sizeof(genTime));
	fprintf(fp, "生成时间: %s\n\n", genTime);
	fprintf(fp, "%-10s %-20s %-15s %-10s %-10s %-10s %-10s\n", "菜品ID", "菜品名称", "所属窗口", "分类", "价格(元)", "销量", "评分");
	fprintf(fp, "--------------------------------------------------------------------------------------\n");

	DishNode* curr = gDishHead;
	while (curr) {
		fprintf(fp, "%-10s %-20s %-15s %-10s %-10.2f %-10d %-10.1f\n",
			curr->data.dishId, curr->data.name, getWindowNameById(curr->data.windowId),
			curr->data.category, curr->data.price, curr->data.sales, curr->data.avgScore);
		curr = curr->next;
	}
	fprintf(fp, "======================================================================================\n");
	fclose(fp);
	showGuiInfo("导出成功", "数据报表已生成！请在程序同级目录下查看：食堂数据报表.txt");
}


/* ================== 【第五部分】 视图渲染层：图形绘制基础 (Views) ================== */

void drawCenteredText(int left, int top, int right, int bottom, const char* text, int fontSize, COLORREF color, int weight) {
	RECT rect = { left, top, right, bottom };
	setbkmode(TRANSPARENT); settextcolor(color); settextstyle(fontSize, 0, "Microsoft YaHei", 0, 0, weight, false, false, false);
	drawtext(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawBackground(void) {
	for (int y = 0; y < GUI_HEIGHT; ++y) {
		int r = max(214, 245 - y / 12), g = max(226, 248 - y / 20), b = max(240, 255 - y / 16);
		setlinecolor(RGB(r, g, b)); line(0, y, GUI_WIDTH, y);
	}
	setfillcolor(RGB(255, 222, 173)); solidcircle(920, 120, 70);
	setfillcolor(RGB(188, 230, 210)); solidcircle(180, 110, 90);
	setfillcolor(RGB(250, 245, 235)); solidroundrect(40, 40, GUI_WIDTH - 40, GUI_HEIGHT - 40, 30, 30);
}

void drawPanel(int left, int top, int right, int bottom, COLORREF fillColor) {
	setfillcolor(fillColor); solidroundrect(left, top, right, bottom, 24, 24);
	setlinecolor(RGB(222, 226, 230)); roundrect(left, top, right, bottom, 24, 24);
}

void drawButton(const Button* btn, int hovered, COLORREF baseColor, COLORREF hoverColor) {
	setfillcolor(hovered ? hoverColor : baseColor);
	solidroundrect(btn->left, btn->top, btn->right, btn->bottom, 20, 20);
	setlinecolor(WHITE); roundrect(btn->left, btn->top, btn->right, btn->bottom, 20, 20);
	drawCenteredText(btn->left, btn->top, btn->right, btn->bottom, btn->text, 24, WHITE, FW_BOLD);
}

void drawTag(int x, int y, const char* text, COLORREF fillColor, COLORREF textColor) {
	int width = textwidth(text) + 26;
	setfillcolor(fillColor); solidroundrect(x, y, x + width, y + 34, 16, 16);
	drawCenteredText(x, y, x + width, y + 34, text, 18, textColor, FW_NORMAL);
}

void drawSummaryCard(int left, int top, int right, int bottom, const char* title, const char* value, COLORREF accent) {
	setfillcolor(WHITE); solidroundrect(left, top, right, bottom, 20, 20);
	setlinecolor(RGB(232, 232, 232)); roundrect(left, top, right, bottom, 20, 20);
	setfillcolor(accent); solidroundrect(left + 18, top + 18, left + 34, bottom - 18, 8, 8);
	drawCenteredText(left + 50, top + 16, right - 18, top + 48, title, 20, RGB(80, 84, 98), FW_NORMAL);
	drawCenteredText(left + 50, top + 46, right - 18, bottom - 12, value, 30, RGB(34, 37, 49), FW_BOLD);
}


/* ================== 【第五部分】 视图渲染层：具体页面 (Views) ================== */

void drawLoginPage(void) {
	drawBackground();
	drawPanel(90, 110, 1010, 630, RGB(252, 252, 252));
	drawTag(145, 145, "主菜单", RGB(229, 243, 255), RGB(40, 88, 140));
	drawCenteredText(130, 185, 980, 255, "西邮食堂菜品管理系统", 38, RGB(30, 37, 54), FW_BOLD);
	drawCenteredText(140, 255, 980, 300, "欢迎使用校园食堂菜品管理系统", 22, RGB(110, 118, 134), FW_NORMAL);
	drawPanel(135, 330, 965, 445, RGB(245, 248, 252));
	drawCenteredText(160, 370, 940, 400, "为保证您的信息安全，输入密码请在黑色控制台窗口进行", 22, RGB(66, 74, 92), FW_BOLD);

	drawButton(&gLoginButtons[0], gHoverIndex == 0, RGB(52, 152, 219), RGB(41, 128, 185));
	drawButton(&gLoginButtons[1], gHoverIndex == 1, RGB(46, 204, 113), RGB(39, 174, 96));
	drawButton(&gLoginButtons[2], gHoverIndex == 2, RGB(243, 156, 18), RGB(211, 84, 0));
	drawButton(&gLoginButtons[3], gHoverIndex == 3, RGB(231, 76, 60), RGB(192, 57, 43));
}

void drawStudentPage(void) {
	char info[64]; drawBackground();
	drawPanel(70, 85, 1030, 640, RGB(251, 251, 251));
	drawTag(105, 115, "学生中心", RGB(230, 248, 240), RGB(34, 98, 62));
	drawCenteredText(100, 150, 980, 210, "学生业务仪表盘", 36, RGB(29, 37, 52), FW_BOLD);
	snprintf(info, sizeof(info), "当前用户: %s", gCurrentUser.username);
	drawCenteredText(100, 210, 980, 245, info, 22, RGB(103, 110, 124), FW_NORMAL);

	drawButton(&gStudentButtons[0], gHoverIndex == 0, RGB(52, 152, 219), RGB(41, 128, 185));
	drawButton(&gStudentButtons[1], gHoverIndex == 1, RGB(26, 188, 156), RGB(22, 160, 133));
	drawButton(&gStudentButtons[2], gHoverIndex == 2, RGB(155, 89, 182), RGB(142, 68, 173));
	drawButton(&gStudentButtons[3], gHoverIndex == 3, RGB(231, 76, 60), RGB(192, 57, 43));

	snprintf(info, sizeof(info), "%d", countDishes());
	drawSummaryCard(470, 245, 680, 360, "菜品总数", info, RGB(52, 152, 219));
	snprintf(info, sizeof(info), "%d", countOrders());
	drawSummaryCard(715, 245, 925, 360, "全站订单", info, RGB(26, 188, 156));
	snprintf(info, sizeof(info), "%d", countComments());
	drawSummaryCard(470, 395, 680, 510, "全站评价", info, RGB(243, 156, 18));
	snprintf(info, sizeof(info), "%d", countWindows());
	drawSummaryCard(715, 395, 925, 510, "营业窗口", info, RGB(155, 89, 182));

	drawPanel(470, 545, 925, 605, RGB(245, 248, 252));
	drawCenteredText(490, 556, 905, 594, "您可以在这里浏览菜品、下单，并对完成的订单进行评价。", 20, RGB(100, 108, 121), FW_NORMAL);
}

void drawAdminPage(void) {
	char info[64]; drawBackground();
	drawPanel(70, 70, 1030, 655, RGB(251, 251, 251));
	drawTag(105, 105, "管理中心", RGB(255, 239, 229), RGB(137, 76, 26));
	drawCenteredText(100, 140, 980, 200, "管理员业务仪表盘", 36, RGB(29, 37, 52), FW_BOLD);
	snprintf(info, sizeof(info), "当前用户: %s", gCurrentUser.username);
	drawCenteredText(100, 200, 980, 235, info, 22, RGB(103, 110, 124), FW_NORMAL);

	drawButton(&gAdminButtons[0], gHoverIndex == 0, RGB(41, 128, 185), RGB(31, 97, 141));
	drawButton(&gAdminButtons[1], gHoverIndex == 1, RGB(39, 174, 96), RGB(30, 132, 73));
	drawButton(&gAdminButtons[2], gHoverIndex == 2, RGB(142, 68, 173), RGB(110, 44, 138));
	drawButton(&gAdminButtons[3], gHoverIndex == 3, RGB(230, 126, 34), RGB(211, 84, 0));
	drawButton(&gAdminButtons[4], gHoverIndex == 4, RGB(192, 57, 43), RGB(169, 50, 38));

	snprintf(info, sizeof(info), "%d", countWindows());
	drawSummaryCard(470, 205, 680, 320, "窗口总数", info, RGB(41, 128, 185));
	snprintf(info, sizeof(info), "%d", countDishes());
	drawSummaryCard(715, 205, 925, 320, "菜品总数", info, RGB(39, 174, 96));
	snprintf(info, sizeof(info), "%d", countUsers());
	drawSummaryCard(470, 355, 680, 470, "注册用户", info, RGB(142, 68, 173));
	snprintf(info, sizeof(info), "%d", countOrders());
	drawSummaryCard(715, 355, 925, 470, "全站订单", info, RGB(230, 126, 34));
	snprintf(info, sizeof(info), "%d", countComments());
	drawSummaryCard(592, 505, 805, 620, "全站评价", info, RGB(231, 76, 60));
}

// 核心通用表格绘制层 - 学生浏览菜品 (带有搜索框)
void drawStudentBrowsePage(void) {
	drawBackground();
	drawCenteredText(0, 20, GUI_WIDTH, 80, "菜品浏览中心", 36, RGB(29, 37, 52), FW_BOLD);

	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));

	// 绘制顶部工具栏按钮
	drawButton(&gBtnSearch, gHoverIndex == 110, RGB(52, 152, 219), RGB(41, 128, 185));
	drawButton(&gBtnComboSearch, gHoverIndex == 112, RGB(155, 89, 182), RGB(142, 68, 173));
	drawButton(&gBtnSortPriceAsc, gHoverIndex == 113, RGB(243, 156, 18), RGB(211, 84, 0));
	drawButton(&gBtnSortPriceDesc, gHoverIndex == 115, RGB(243, 156, 18), RGB(211, 84, 0));
	drawButton(&gBtnSortSalesDesc, gHoverIndex == 114, RGB(231, 76, 60), RGB(192, 57, 43));

	// 如果有搜索条件，才显示“清空条件”按钮
	if (gSearchKeyword[0] != '\0' || gComboSearchWindow[0] != '\0') {
		drawButton(&gBtnClearSearch, gHoverIndex == 111, RGB(149, 165, 166), RGB(127, 140, 141));
	}

	// 表格整体往下移到 Y=140
	int startY = 140, rowHeight = 40;
	setfillcolor(RGB(229, 243, 255)); solidrectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	setlinecolor(RGB(189, 195, 199)); rectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	drawCenteredText(50, startY, 150, startY + rowHeight, "菜品 ID", 18, BLACK, FW_BOLD);
	drawCenteredText(150, startY, 350, startY + rowHeight, "菜品名称", 18, BLACK, FW_BOLD);
	drawCenteredText(350, startY, 450, startY + rowHeight, "分类", 18, BLACK, FW_BOLD);
	drawCenteredText(450, startY, 550, startY + rowHeight, "价格", 18, BLACK, FW_BOLD);
	drawCenteredText(550, startY, 650, startY + rowHeight, "库存", 18, BLACK, FW_BOLD);
	drawCenteredText(650, startY, 800, startY + rowHeight, "所属窗口", 18, BLACK, FW_BOLD);
	drawCenteredText(800, startY, 900, startY + rowHeight, "评分", 18, BLACK, FW_BOLD);
	drawCenteredText(900, startY, 1050, startY + rowHeight, "销量", 18, BLACK, FW_BOLD);

	int totalFilteredItems = countFilteredDishes();
	gTableTotalPages = (totalFilteredItems + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (gTableTotalPages == 0) gTableTotalPages = 1;
	if (gTableCurrentPage > gTableTotalPages) gTableCurrentPage = gTableTotalPages;

	DishNode* current = gDishHead; int skipped = 0;
	while (current && skipped < (gTableCurrentPage - 1) * ITEMS_PER_PAGE) {
		bool matchName = (gSearchKeyword[0] == '\0' || strstr(current->data.name, gSearchKeyword) != NULL);
		bool matchWindow = (gComboSearchWindow[0] == '\0' || strcmp(current->data.windowId, gComboSearchWindow) == 0);
		bool matchCategory = (gComboSearchCategory[0] == '\0' || strcmp(current->data.category, gComboSearchCategory) == 0);
		if (matchName && matchWindow && matchCategory) skipped++;
		current = current->next;
	}

	char buffer[64]; int drawn = 0;
	while (current && drawn < ITEMS_PER_PAGE) {
		bool matchName = (gSearchKeyword[0] == '\0' || strstr(current->data.name, gSearchKeyword) != NULL);
		bool matchWindow = (gComboSearchWindow[0] == '\0' || strcmp(current->data.windowId, gComboSearchWindow) == 0);
		bool matchCategory = (gComboSearchCategory[0] == '\0' || strcmp(current->data.category, gComboSearchCategory) == 0);

		if (matchName && matchWindow && matchCategory) {
			int y = startY + (drawn + 1) * rowHeight;
			setfillcolor(drawn % 2 == 0 ? RGB(250, 250, 250) : RGB(240, 240, 240));
			solidrectangle(50, y, GUI_WIDTH - 50, y + rowHeight);
			rectangle(50, y, GUI_WIDTH - 50, y + rowHeight);
			drawCenteredText(50, y, 150, y + rowHeight, current->data.dishId, 18, BLACK, FW_NORMAL);
			drawCenteredText(150, y, 350, y + rowHeight, current->data.name, 18, BLACK, FW_NORMAL);
			drawCenteredText(350, y, 450, y + rowHeight, current->data.category, 18, BLACK, FW_NORMAL);
			snprintf(buffer, sizeof(buffer), "%.2f", current->data.price); drawCenteredText(450, y, 550, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
			snprintf(buffer, sizeof(buffer), "%d", current->data.stock); drawCenteredText(550, y, 650, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
			drawCenteredText(650, y, 800, y + rowHeight, getWindowNameById(current->data.windowId), 18, BLACK, FW_NORMAL);
			snprintf(buffer, sizeof(buffer), "%.1f (%d人)", current->data.avgScore, current->data.scoreCount); drawCenteredText(800, y, 900, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
			snprintf(buffer, sizeof(buffer), "%d", current->data.sales); drawCenteredText(900, y, 1050, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
			drawn++;
		}
		current = current->next;
	}
	snprintf(buffer, sizeof(buffer), "第 %d / %d 页", gTableCurrentPage, gTableTotalPages);
	drawCenteredText(500, 650, 600, 690, buffer, 20, BLACK, FW_BOLD);
	drawButton(&gBtnPrevPage, gHoverIndex == 100, RGB(52, 152, 219), RGB(41, 128, 185));
	drawButton(&gBtnNextPage, gHoverIndex == 101, RGB(52, 152, 219), RGB(41, 128, 185));
}

void drawStudentMyOrdersPage(void) {
	drawBackground();
	drawCenteredText(0, 20, GUI_WIDTH, 80, "我的订单中心", 36, RGB(29, 37, 52), FW_BOLD);
	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));

	int startY = 120, rowHeight = 40;
	setfillcolor(RGB(230, 248, 240)); solidrectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	setlinecolor(RGB(189, 195, 199)); rectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	drawCenteredText(50, startY, 200, startY + rowHeight, "订单 ID", 18, BLACK, FW_BOLD);
	drawCenteredText(200, startY, 400, startY + rowHeight, "菜品名称", 18, BLACK, FW_BOLD);
	drawCenteredText(400, startY, 500, startY + rowHeight, "数量", 18, BLACK, FW_BOLD);
	drawCenteredText(500, startY, 650, startY + rowHeight, "总价(元)", 18, BLACK, FW_BOLD);
	drawCenteredText(650, startY, 800, startY + rowHeight, "状态", 18, BLACK, FW_BOLD);
	drawCenteredText(800, startY, 1050, startY + rowHeight, "下单时间", 18, BLACK, FW_BOLD);

	int userOrderCount = 0; OrderNode* temp = gOrderHead;
	while (temp) { if (strcmp(temp->data.username, gCurrentUser.username) == 0) userOrderCount++; temp = temp->next; }
	gOrderTableTotalPages = (userOrderCount + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (gOrderTableTotalPages == 0) gOrderTableTotalPages = 1;
	if (gOrderTableCurrentPage > gOrderTableTotalPages) gOrderTableCurrentPage = gOrderTableTotalPages;

	OrderNode* current = gOrderHead; int skipped = 0;
	while (current && skipped < (gOrderTableCurrentPage - 1) * ITEMS_PER_PAGE) {
		if (strcmp(current->data.username, gCurrentUser.username) == 0) skipped++;
		current = current->next;
	}

	char buffer[64]; int drawn = 0;
	while (current && drawn < ITEMS_PER_PAGE) {
		if (strcmp(current->data.username, gCurrentUser.username) == 0) {
			int y = startY + (drawn + 1) * rowHeight;
			setfillcolor(drawn % 2 == 0 ? RGB(250, 250, 250) : RGB(240, 240, 240));
			solidrectangle(50, y, GUI_WIDTH - 50, y + rowHeight);
			rectangle(50, y, GUI_WIDTH - 50, y + rowHeight);
			drawCenteredText(50, y, 200, y + rowHeight, current->data.orderId, 18, BLACK, FW_NORMAL);
			drawCenteredText(200, y, 400, y + rowHeight, current->data.dishName, 18, BLACK, FW_NORMAL);
			snprintf(buffer, sizeof(buffer), "%d", current->data.quantity); drawCenteredText(400, y, 500, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
			snprintf(buffer, sizeof(buffer), "%.2f", current->data.totalPrice); drawCenteredText(500, y, 650, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);

			COLORREF statusColor = current->data.status == ORDER_COMPLETED ? RGB(39, 174, 96) : (current->data.status == ORDER_PENDING ? RGB(230, 126, 34) : RGB(142, 68, 173));
			drawCenteredText(650, y, 800, y + rowHeight, getOrderStatusText(current->data.status), 18, statusColor, FW_BOLD);
			drawCenteredText(800, y, 1050, y + rowHeight, current->data.orderTime, 18, BLACK, FW_NORMAL);
			drawn++;
		}
		current = current->next;
	}
	snprintf(buffer, sizeof(buffer), "第 %d / %d 页", gOrderTableCurrentPage, gOrderTableTotalPages);
	drawCenteredText(500, 650, 600, 690, buffer, 20, BLACK, FW_BOLD);
	drawButton(&gBtnPrevPage, gHoverIndex == 100, RGB(26, 188, 156), RGB(22, 160, 133));
	drawButton(&gBtnNextPage, gHoverIndex == 101, RGB(26, 188, 156), RGB(22, 160, 133));
	drawButton(&gBtnSubmitComment, gHoverIndex == 102, RGB(243, 156, 18), RGB(211, 84, 0));
}

void drawAdminWindowPage(void) {
	drawBackground();
	drawCenteredText(0, 20, GUI_WIDTH, 80, "窗口管理中心", 36, RGB(29, 37, 52), FW_BOLD);
	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));

	int startY = 120; int rowHeight = 40;
	setfillcolor(RGB(229, 243, 255)); solidrectangle(150, startY, GUI_WIDTH - 150, startY + rowHeight);
	setlinecolor(RGB(189, 195, 199)); rectangle(150, startY, GUI_WIDTH - 150, startY + rowHeight);
	drawCenteredText(150, startY, 400, startY + rowHeight, "窗口 ID", 18, BLACK, FW_BOLD);
	drawCenteredText(400, startY, GUI_WIDTH - 150, startY + rowHeight, "窗口名称", 18, BLACK, FW_BOLD);

	gAdminWindowTotal = (countWindows() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (gAdminWindowTotal == 0) gAdminWindowTotal = 1;
	if (gAdminWindowPage > gAdminWindowTotal) gAdminWindowPage = gAdminWindowTotal;

	WindowNode* current = gWindowHead;
	for (int i = 0; i < (gAdminWindowPage - 1) * ITEMS_PER_PAGE && current != NULL; i++) current = current->next;

	for (int i = 0; i < ITEMS_PER_PAGE && current != NULL; i++) {
		int y = startY + (i + 1) * rowHeight;
		setfillcolor(i % 2 == 0 ? RGB(250, 250, 250) : RGB(240, 240, 240));
		solidrectangle(150, y, GUI_WIDTH - 150, y + rowHeight);
		rectangle(150, y, GUI_WIDTH - 150, y + rowHeight);
		drawCenteredText(150, y, 400, y + rowHeight, current->data.windowId, 18, BLACK, FW_NORMAL);
		drawCenteredText(400, y, GUI_WIDTH - 150, y + rowHeight, current->data.windowName, 18, BLACK, FW_NORMAL);
		current = current->next;
	}

	char buffer[64]; snprintf(buffer, sizeof(buffer), "第 %d / %d 页", gAdminWindowPage, gAdminWindowTotal);
	drawCenteredText(500, 650, 600, 690, buffer, 20, BLACK, FW_BOLD);
	drawButton(&gBtnPrevPage, gHoverIndex == 100, RGB(41, 128, 185), RGB(31, 97, 141));
	drawButton(&gBtnNextPage, gHoverIndex == 101, RGB(41, 128, 185), RGB(31, 97, 141));
	drawButton(&gBtnAdd, gHoverIndex == 102, RGB(39, 174, 96), RGB(30, 132, 73));
	drawButton(&gBtnDelete, gHoverIndex == 104, RGB(231, 76, 60), RGB(192, 57, 43));
}

void drawAdminDishPage(void) {
	drawBackground();
	drawCenteredText(0, 20, GUI_WIDTH, 80, "菜品管理中心", 36, RGB(29, 37, 52), FW_BOLD);
	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));

	int startY = 120, rowHeight = 40;
	setfillcolor(RGB(229, 243, 255)); solidrectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	setlinecolor(RGB(189, 195, 199)); rectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	drawCenteredText(50, startY, 150, startY + rowHeight, "菜品 ID", 18, BLACK, FW_BOLD);
	drawCenteredText(150, startY, 350, startY + rowHeight, "菜品名称", 18, BLACK, FW_BOLD);
	drawCenteredText(350, startY, 450, startY + rowHeight, "分类", 18, BLACK, FW_BOLD);
	drawCenteredText(450, startY, 550, startY + rowHeight, "价格", 18, BLACK, FW_BOLD);
	drawCenteredText(550, startY, 650, startY + rowHeight, "库存", 18, BLACK, FW_BOLD);
	drawCenteredText(650, startY, 800, startY + rowHeight, "所属窗口", 18, BLACK, FW_BOLD);
	drawCenteredText(800, startY, 900, startY + rowHeight, "评分", 18, BLACK, FW_BOLD);
	drawCenteredText(900, startY, 1050, startY + rowHeight, "销量", 18, BLACK, FW_BOLD);

	gAdminDishTotal = (countDishes() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (gAdminDishTotal == 0) gAdminDishTotal = 1;
	if (gAdminDishPage > gAdminDishTotal) gAdminDishPage = gAdminDishTotal;

	DishNode* current = gDishHead;
	for (int i = 0; i < (gAdminDishPage - 1) * ITEMS_PER_PAGE && current != NULL; i++) current = current->next;

	char buffer[64];
	for (int i = 0; i < ITEMS_PER_PAGE && current != NULL; i++) {
		int y = startY + (i + 1) * rowHeight;
		setfillcolor(i % 2 == 0 ? RGB(250, 250, 250) : RGB(240, 240, 240));
		solidrectangle(50, y, GUI_WIDTH - 50, y + rowHeight); rectangle(50, y, GUI_WIDTH - 50, y + rowHeight);
		drawCenteredText(50, y, 150, y + rowHeight, current->data.dishId, 18, BLACK, FW_NORMAL);
		drawCenteredText(150, y, 350, y + rowHeight, current->data.name, 18, BLACK, FW_NORMAL);
		drawCenteredText(350, y, 450, y + rowHeight, current->data.category, 18, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "%.2f", current->data.price); drawCenteredText(450, y, 550, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "%d", current->data.stock); drawCenteredText(550, y, 650, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
		drawCenteredText(650, y, 800, y + rowHeight, getWindowNameById(current->data.windowId), 18, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "%.1f (%d人)", current->data.avgScore, current->data.scoreCount); drawCenteredText(800, y, 900, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "%d", current->data.sales); drawCenteredText(900, y, 1050, y + rowHeight, buffer, 18, BLACK, FW_NORMAL);
		current = current->next;
	}

	snprintf(buffer, sizeof(buffer), "第 %d / %d 页", gAdminDishPage, gAdminDishTotal);
	drawCenteredText(500, 650, 600, 690, buffer, 20, BLACK, FW_BOLD);
	// 在 drawAdminDishPage 底部替换：
	drawButton(&gBtnPrevPage, gHoverIndex == 100, RGB(41, 128, 185), RGB(31, 97, 141));
	drawButton(&gBtnNextPage, gHoverIndex == 101, RGB(41, 128, 185), RGB(31, 97, 141));
	drawButton(&gBtnAdd, gHoverIndex == 102, RGB(39, 174, 96), RGB(30, 132, 73));
	drawButton(&gBtnEdit, gHoverIndex == 103, RGB(243, 156, 18), RGB(211, 84, 0)); // 修改按钮
	drawButton(&gBtnDelete, gHoverIndex == 104, RGB(231, 76, 60), RGB(192, 57, 43));
	
}

void drawAdminOrderPage(void) {
	drawBackground();
	drawCenteredText(0, 20, GUI_WIDTH, 80, "全站订单管理", 36, RGB(29, 37, 52), FW_BOLD);
	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));

	int startY = 120; int rowHeight = 40;
	setfillcolor(RGB(255, 239, 229)); solidrectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	setlinecolor(RGB(189, 195, 199)); rectangle(50, startY, GUI_WIDTH - 50, startY + rowHeight);
	drawCenteredText(50, startY, 180, startY + rowHeight, "订单号", 18, BLACK, FW_BOLD);
	drawCenteredText(180, startY, 300, startY + rowHeight, "用户名", 18, BLACK, FW_BOLD);
	drawCenteredText(300, startY, 480, startY + rowHeight, "菜品名称", 18, BLACK, FW_BOLD);
	drawCenteredText(480, startY, 560, startY + rowHeight, "数量", 18, BLACK, FW_BOLD);
	drawCenteredText(560, startY, 660, startY + rowHeight, "金额(元)", 18, BLACK, FW_BOLD);
	drawCenteredText(660, startY, 800, startY + rowHeight, "状态", 18, BLACK, FW_BOLD);
	drawCenteredText(800, startY, 1050, startY + rowHeight, "时间", 18, BLACK, FW_BOLD);

	gAdminOrderTotal = (countOrders() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
	if (gAdminOrderTotal == 0) gAdminOrderTotal = 1;
	if (gAdminOrderPage > gAdminOrderTotal) gAdminOrderPage = gAdminOrderTotal;

	OrderNode* current = gOrderHead;
	for (int i = 0; i < (gAdminOrderPage - 1) * ITEMS_PER_PAGE && current != NULL; i++) current = current->next;

	char buffer[64];
	for (int i = 0; i < ITEMS_PER_PAGE && current != NULL; i++) {
		int y = startY + (i + 1) * rowHeight;
		setfillcolor(i % 2 == 0 ? RGB(250, 250, 250) : RGB(240, 240, 240));
		solidrectangle(50, y, GUI_WIDTH - 50, y + rowHeight); rectangle(50, y, GUI_WIDTH - 50, y + rowHeight);
		drawCenteredText(50, y, 180, y + rowHeight, current->data.orderId, 16, BLACK, FW_NORMAL);
		drawCenteredText(180, y, 300, y + rowHeight, current->data.username, 16, BLACK, FW_NORMAL);
		drawCenteredText(300, y, 480, y + rowHeight, current->data.dishName, 16, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "%d", current->data.quantity); drawCenteredText(480, y, 560, y + rowHeight, buffer, 16, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "%.2f", current->data.totalPrice); drawCenteredText(560, y, 660, y + rowHeight, buffer, 16, BLACK, FW_NORMAL);

		COLORREF stColor = (current->data.status == ORDER_PENDING) ? RGB(231, 76, 60) : RGB(39, 174, 96);
		drawCenteredText(660, y, 800, y + rowHeight, getOrderStatusText(current->data.status), 16, stColor, FW_BOLD);
		drawCenteredText(800, y, 1050, y + rowHeight, current->data.orderTime, 16, BLACK, FW_NORMAL);
		current = current->next;
	}

	snprintf(buffer, sizeof(buffer), "第 %d / %d 页", gAdminOrderPage, gAdminOrderTotal);
	drawCenteredText(500, 650, 600, 690, buffer, 20, BLACK, FW_BOLD);
	drawButton(&gBtnPrevPage, gHoverIndex == 100, RGB(142, 68, 173), RGB(110, 44, 138));
	drawButton(&gBtnNextPage, gHoverIndex == 101, RGB(142, 68, 173), RGB(110, 44, 138));
	drawButton(&gBtnUpdateStatus, gHoverIndex == 105, RGB(243, 156, 18), RGB(211, 84, 0));
}

void drawAdminStatPage(void) {
	drawBackground();
	drawCenteredText(0, 20, GUI_WIDTH, 80, "菜品销量统计 (柱状图)", 36, RGB(29, 37, 52), FW_BOLD);
	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));

	int chartLeft = 120, chartRight = 980, chartTop = 150, chartBottom = 550;
	int maxHeight = chartBottom - chartTop;

	// 背景参考线
	setlinecolor(RGB(220, 220, 220)); setlinestyle(PS_DOT, 1);
	for (int i = 0; i <= 4; i++) { int y = chartBottom - i * (maxHeight / 4); line(chartLeft, y, chartRight, y); }
	setlinestyle(PS_SOLID, 1);

	setlinecolor(RGB(100, 100, 100));
	line(chartLeft, chartTop, chartLeft, chartBottom);
	line(chartLeft, chartBottom, chartRight, chartBottom);

	gAdminStatTotal = (countDishes() + BARS_PER_PAGE - 1) / BARS_PER_PAGE;
	if (gAdminStatTotal == 0) gAdminStatTotal = 1;
	if (gAdminStatPage > gAdminStatTotal) gAdminStatPage = gAdminStatTotal;

	DishNode* current = gDishHead;
	for (int i = 0; i < (gAdminStatPage - 1) * BARS_PER_PAGE && current != NULL; i++) current = current->next;

	int maxSales = getGlobalMaxSales();
	int barSpacing = (chartRight - chartLeft) / BARS_PER_PAGE;
	int barWidth = 60; char buffer[64];

	for (int i = 0; i < BARS_PER_PAGE && current != NULL; i++) {
		int centerX = chartLeft + i * barSpacing + barSpacing / 2;
		int barLeft = centerX - barWidth / 2, barRight = centerX + barWidth / 2;
		int barHeight = (int)((double)current->data.sales / maxSales * maxHeight);
		if (barHeight == 0) barHeight = 2; // 避免0销量没有柱子
		int barTop = chartBottom - barHeight;

		setfillcolor(RGB(52, 152, 219)); solidrectangle(barLeft, barTop, barRight, chartBottom);
		setlinecolor(RGB(41, 128, 185)); rectangle(barLeft, barTop, barRight, chartBottom);

		snprintf(buffer, sizeof(buffer), "%d 份", current->data.sales);
		drawCenteredText(barLeft - 20, barTop - 30, barRight + 20, barTop, buffer, 16, RGB(231, 76, 60), FW_BOLD);
		drawCenteredText(barLeft - 30, chartBottom + 10, barRight + 30, chartBottom + 40, current->data.name, 16, BLACK, FW_NORMAL);
		snprintf(buffer, sizeof(buffer), "评分:%.1f", current->data.avgScore);
		drawCenteredText(barLeft - 30, chartBottom + 40, barRight + 30, chartBottom + 60, buffer, 14, RGB(100, 100, 100), FW_NORMAL);

		current = current->next;
	}

	snprintf(buffer, sizeof(buffer), "图表 第 %d / %d 页", gAdminStatPage, gAdminStatTotal);
	// 在 drawAdminStatPage 顶部添加：
	drawCenteredText(0, 20, GUI_WIDTH, 80, "菜品销量统计 (柱状图)", 36, RGB(29, 37, 52), FW_BOLD);
	drawButton(&gBtnBack, gHoverIndex == 99, RGB(189, 195, 199), RGB(149, 165, 166));
	drawButton(&gBtnExport, gHoverIndex == 115, RGB(46, 204, 113), RGB(39, 174, 96)); // 导出报表按钮
}

// 渲染总调度入口
void drawCurrentPage(void) {
	cleardevice();
	if (gCurrentPage == PAGE_LOGIN) drawLoginPage();
	else if (gCurrentPage == PAGE_STUDENT) drawStudentPage();
	else if (gCurrentPage == PAGE_STUDENT_BROWSE) drawStudentBrowsePage();
	else if (gCurrentPage == PAGE_STUDENT_MY_ORDERS) drawStudentMyOrdersPage();
	else if (gCurrentPage == PAGE_ADMIN) drawAdminPage();
	else if (gCurrentPage == PAGE_ADMIN_WINDOW) drawAdminWindowPage();
	else if (gCurrentPage == PAGE_ADMIN_ORDER) drawAdminOrderPage();
	else if (gCurrentPage == PAGE_ADMIN_DISH) drawAdminDishPage();
	else if (gCurrentPage == PAGE_ADMIN_STAT) drawAdminStatPage();
	FlushBatchDraw();
}

/* ================== 【第六部分】 控制层：事件监听与路由 (Controllers) ================== */

int pointInButton(int x, int y, const Button* btn) {
	return x >= btn->left && x <= btn->right && y >= btn->top && y <= btn->bottom;
}

void handleLoginPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		for (int i = 0; i < 4; ++i) if (pointInButton(msg->x, msg->y, &gLoginButtons[i])) { gHoverIndex = i; break; }
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gLoginButtons[0])) doStudentLogin();
		else if (pointInButton(msg->x, msg->y, &gLoginButtons[1])) doAdminLogin();
		else if (pointInButton(msg->x, msg->y, &gLoginButtons[2])) doStudentRegister();
		else if (pointInButton(msg->x, msg->y, &gLoginButtons[3])) {
			if (MessageBox(GetHWnd(), "确定要退出西邮食堂菜品管理系统吗？", "退出确认", MB_YESNO | MB_ICONQUESTION) == IDYES) gRunning = 0;
		}
		drawCurrentPage();
	}
}

void handleStudentPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		for (int i = 0; i < 4; ++i) if (pointInButton(msg->x, msg->y, &gStudentButtons[i])) { gHoverIndex = i; break; }
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gStudentButtons[0])) { gCurrentPage = PAGE_STUDENT_BROWSE; gTableCurrentPage = 1; }
		else if (pointInButton(msg->x, msg->y, &gStudentButtons[1])) doGuiPlaceOrder();
		else if (pointInButton(msg->x, msg->y, &gStudentButtons[2])) { gCurrentPage = PAGE_STUDENT_MY_ORDERS; gOrderTableCurrentPage = 1; }
		else if (pointInButton(msg->x, msg->y, &gStudentButtons[3])) { logoutCurrentUser(); showGuiInfo("注销", "您已成功注销登录。"); }
		drawCurrentPage();
	}
}

void handleStudentBrowsePage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		if (pointInButton(msg->x, msg->y, &gBtnBack)) gHoverIndex = 99;
		if (pointInButton(msg->x, msg->y, &gBtnPrevPage)) gHoverIndex = 100;
		if (pointInButton(msg->x, msg->y, &gBtnNextPage)) gHoverIndex = 101;

		// 工具栏悬停效果
		if (pointInButton(msg->x, msg->y, &gBtnSearch)) gHoverIndex = 110;
		if (pointInButton(msg->x, msg->y, &gBtnComboSearch)) gHoverIndex = 112;
		if (pointInButton(msg->x, msg->y, &gBtnSortPriceAsc)) gHoverIndex = 113;
		if (pointInButton(msg->x, msg->y, &gBtnSortPriceDesc)) gHoverIndex = 115;
		if (pointInButton(msg->x, msg->y, &gBtnSortSalesDesc)) gHoverIndex = 114;
		if ((gSearchKeyword[0] != '\0' || gComboSearchWindow[0] != '\0') && pointInButton(msg->x, msg->y, &gBtnClearSearch)) gHoverIndex = 111;

		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gBtnBack)) {
			gCurrentPage = PAGE_STUDENT; gSearchKeyword[0] = '\0'; gComboSearchWindow[0] = '\0'; gComboSearchCategory[0] = '\0'; gHoverIndex = -1;
		}
		else if (pointInButton(msg->x, msg->y, &gBtnPrevPage) && gTableCurrentPage > 1) gTableCurrentPage--;
		else if (pointInButton(msg->x, msg->y, &gBtnNextPage) && gTableCurrentPage < gTableTotalPages) gTableCurrentPage++;
		else if (pointInButton(msg->x, msg->y, &gBtnSearch)) {
			char kw[50] = "";
			if (InputBox(kw, 50, "请输入要搜索的菜品名称关键字:", "名称模糊搜索") && !stringEmpty(kw)) {
				copyText(gSearchKeyword, kw, sizeof(gSearchKeyword)); gTableCurrentPage = 1;
			}
		}
		else if (pointInButton(msg->x, msg->y, &gBtnComboSearch)) {
			char wId[20] = "", ctg[20] = "";
			InputBox(wId, 20, "1/2 请输入窗口ID (留空则不限):", "组合查询");
			InputBox(ctg, 20, "2/2 请输入菜品分类 (留空则不限):", "组合查询");
			copyText(gComboSearchWindow, wId, sizeof(gComboSearchWindow));
			copyText(gComboSearchCategory, ctg, sizeof(gComboSearchCategory));
			gTableCurrentPage = 1;
		}
		// --- 排序点击触发 ---
		else if (pointInButton(msg->x, msg->y, &gBtnSortPriceAsc)) {
			sortDishesByPriceAsc(); showGuiInfo("排序完成", "已按价格【升序】排列！");
		}
		else if (pointInButton(msg->x, msg->y, &gBtnSortPriceDesc)) {
			sortDishesByPriceDesc(); showGuiInfo("排序完成", "已按价格【降序】排列！");
		}
		else if (pointInButton(msg->x, msg->y, &gBtnSortSalesDesc)) {
			sortDishesBySalesDesc(); showGuiInfo("排序完成", "已按销量【降序】排列！");
		}
		// --- 清除条件 ---
		else if ((gSearchKeyword[0] != '\0' || gComboSearchWindow[0] != '\0') && pointInButton(msg->x, msg->y, &gBtnClearSearch)) {
			gSearchKeyword[0] = '\0'; gComboSearchWindow[0] = '\0'; gComboSearchCategory[0] = '\0'; gTableCurrentPage = 1;
		}
		drawCurrentPage();
	}
}

void handleStudentMyOrdersPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		if (pointInButton(msg->x, msg->y, &gBtnBack)) gHoverIndex = 99;
		if (pointInButton(msg->x, msg->y, &gBtnPrevPage)) gHoverIndex = 100;
		if (pointInButton(msg->x, msg->y, &gBtnNextPage)) gHoverIndex = 101;
		if (pointInButton(msg->x, msg->y, &gBtnSubmitComment)) gHoverIndex = 102;
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gBtnBack)) { gCurrentPage = PAGE_STUDENT; gHoverIndex = -1; }
		else if (pointInButton(msg->x, msg->y, &gBtnPrevPage) && gOrderTableCurrentPage > 1) gOrderTableCurrentPage--;
		else if (pointInButton(msg->x, msg->y, &gBtnNextPage) && gOrderTableCurrentPage < gOrderTableTotalPages) gOrderTableCurrentPage++;
		else if (pointInButton(msg->x, msg->y, &gBtnSubmitComment)) doGuiSubmitComment();
		drawCurrentPage();
	}
}

void handleAdminPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		for (int i = 0; i < 5; ++i) if (pointInButton(msg->x, msg->y, &gAdminButtons[i])) { gHoverIndex = i; break; }
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gAdminButtons[0])) { gCurrentPage = PAGE_ADMIN_WINDOW; gAdminWindowPage = 1; }
		else if (pointInButton(msg->x, msg->y, &gAdminButtons[1])) { gCurrentPage = PAGE_ADMIN_DISH; gAdminDishPage = 1; }
		else if (pointInButton(msg->x, msg->y, &gAdminButtons[2])) { gCurrentPage = PAGE_ADMIN_ORDER; gAdminOrderPage = 1; }
		else if (pointInButton(msg->x, msg->y, &gAdminButtons[3])) { gCurrentPage = PAGE_ADMIN_STAT; gAdminStatPage = 1; }
		else if (pointInButton(msg->x, msg->y, &gAdminButtons[4])) { logoutCurrentUser(); showGuiInfo("注销", "管理员已注销登录。"); }
		drawCurrentPage();
	}
}

void handleAdminWindowPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		if (pointInButton(msg->x, msg->y, &gBtnBack)) gHoverIndex = 99;
		if (pointInButton(msg->x, msg->y, &gBtnPrevPage)) gHoverIndex = 100;
		if (pointInButton(msg->x, msg->y, &gBtnNextPage)) gHoverIndex = 101;
		if (pointInButton(msg->x, msg->y, &gBtnAdd)) gHoverIndex = 102;
		if (pointInButton(msg->x, msg->y, &gBtnDelete)) gHoverIndex = 104;
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gBtnBack)) { gCurrentPage = PAGE_ADMIN; gHoverIndex = -1; }
		else if (pointInButton(msg->x, msg->y, &gBtnPrevPage) && gAdminWindowPage > 1) gAdminWindowPage--;
		else if (pointInButton(msg->x, msg->y, &gBtnNextPage) && gAdminWindowPage < gAdminWindowTotal) gAdminWindowPage++;
		else if (pointInButton(msg->x, msg->y, &gBtnAdd)) doGuiAddWindow();
		else if (pointInButton(msg->x, msg->y, &gBtnDelete)) doGuiDeleteWindow();
		drawCurrentPage();
	}
}

void handleAdminDishPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		if (pointInButton(msg->x, msg->y, &gBtnBack)) gHoverIndex = 99;
		if (pointInButton(msg->x, msg->y, &gBtnPrevPage)) gHoverIndex = 100;
		if (pointInButton(msg->x, msg->y, &gBtnNextPage)) gHoverIndex = 101;
		if (pointInButton(msg->x, msg->y, &gBtnAdd)) gHoverIndex = 102;
		if (pointInButton(msg->x, msg->y, &gBtnDelete)) gHoverIndex = 104;
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gBtnBack)) { gCurrentPage = PAGE_ADMIN; gHoverIndex = -1; }
		else if (pointInButton(msg->x, msg->y, &gBtnPrevPage) && gAdminDishPage > 1) gAdminDishPage--;
		else if (pointInButton(msg->x, msg->y, &gBtnNextPage) && gAdminDishPage < gAdminDishTotal) gAdminDishPage++;
		else if (pointInButton(msg->x, msg->y, &gBtnAdd)) doGuiAddDish();
		else if (pointInButton(msg->x, msg->y, &gBtnDelete)) doGuiDeleteDish();
		// 在 handleAdminDishPage 的 WM_LBUTTONDOWN 中增加：
		else if (pointInButton(msg->x, msg->y, &gBtnEdit)) doGuiModifyDish();
		drawCurrentPage();
	}
}

void handleAdminOrderPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		if (pointInButton(msg->x, msg->y, &gBtnBack)) gHoverIndex = 99;
		if (pointInButton(msg->x, msg->y, &gBtnPrevPage)) gHoverIndex = 100;
		if (pointInButton(msg->x, msg->y, &gBtnNextPage)) gHoverIndex = 101;
		if (pointInButton(msg->x, msg->y, &gBtnUpdateStatus)) gHoverIndex = 105;
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gBtnBack)) { gCurrentPage = PAGE_ADMIN; gHoverIndex = -1; }
		else if (pointInButton(msg->x, msg->y, &gBtnPrevPage) && gAdminOrderPage > 1) gAdminOrderPage--;
		else if (pointInButton(msg->x, msg->y, &gBtnNextPage) && gAdminOrderPage < gAdminOrderTotal) gAdminOrderPage++;
		else if (pointInButton(msg->x, msg->y, &gBtnUpdateStatus)) doGuiUpdateOrderStatus();
		drawCurrentPage();
	}
}

void handleAdminStatPage(const MOUSEMSG* msg) {
	if (msg->uMsg == WM_MOUSEMOVE) {
		gHoverIndex = -1;
		if (pointInButton(msg->x, msg->y, &gBtnBack)) gHoverIndex = 99;
		if (pointInButton(msg->x, msg->y, &gBtnPrevPage)) gHoverIndex = 100;
		if (pointInButton(msg->x, msg->y, &gBtnNextPage)) gHoverIndex = 101;
		drawCurrentPage();
	}
	else if (msg->uMsg == WM_LBUTTONDOWN) {
		if (pointInButton(msg->x, msg->y, &gBtnBack)) { gCurrentPage = PAGE_ADMIN; gHoverIndex = -1; }
		else if (pointInButton(msg->x, msg->y, &gBtnPrevPage) && gAdminStatPage > 1) gAdminStatPage--;
		else if (pointInButton(msg->x, msg->y, &gBtnNextPage) && gAdminStatPage < gAdminStatTotal) gAdminStatPage++;
		// 在 handleAdminStatPage 的 WM_LBUTTONDOWN 中增加：
		else if (pointInButton(msg->x, msg->y, &gBtnExport)) doGuiExportReport();
		drawCurrentPage();
	}
}

/* ================== 【主程序入口】 ================== */

void consoleHeader(const char* title) {
	system("cls");
	printf("============================================================\n");
	printf("%s\n", title);
	printf("============================================================\n");
}

int main(void) {
	MOUSEMSG msg;
	setlocale(LC_ALL, "");
	SetConsoleOutputCP(65001); // UTF-8 控制台兼容
	SetConsoleCP(65001);
	atexit(cleanup);

	memset(&gCurrentUser, 0, sizeof(gCurrentUser));
	loadAllData();
	initializeDefaultData();

	initgraph(GUI_WIDTH, GUI_HEIGHT, SHOWCONSOLE);
	BeginBatchDraw();
	drawCurrentPage();

	consoleHeader("系统已就绪");
	printf("西邮食堂餐品管理系统已打开。\n");
	printf("请使用 EasyX 图形化界面进行操作。\n");
	printf("安全提示: 若要输入登录注册密码，请在此黑色控制台窗口中进行。\n");

	while (gRunning) {
		if (MouseHit()) {
			msg = GetMouseMsg();
			if (gCurrentPage == PAGE_LOGIN) handleLoginPage(&msg);
			else if (gCurrentPage == PAGE_STUDENT) handleStudentPage(&msg);
			else if (gCurrentPage == PAGE_ADMIN) handleAdminPage(&msg);
			else if (gCurrentPage == PAGE_STUDENT_BROWSE) handleStudentBrowsePage(&msg);
			else if (gCurrentPage == PAGE_STUDENT_MY_ORDERS) handleStudentMyOrdersPage(&msg);
			else if (gCurrentPage == PAGE_ADMIN_WINDOW) handleAdminWindowPage(&msg);
			else if (gCurrentPage == PAGE_ADMIN_ORDER) handleAdminOrderPage(&msg);
			else if (gCurrentPage == PAGE_ADMIN_DISH) handleAdminDishPage(&msg);
			else if (gCurrentPage == PAGE_ADMIN_STAT) handleAdminStatPage(&msg);
		}

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			if (MessageBox(GetHWnd(), "确定要退出系统吗？", "快捷退出确认", MB_YESNO | MB_ICONQUESTION) == IDYES) gRunning = 0;
			Sleep(200);
		}
		Sleep(10); // 降低 CPU 占用
	}

	EndBatchDraw();
	closegraph();
	return 0;
}