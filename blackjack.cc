#include <cstring>
#include <random>
#include <vector>
#include <string>
#include <unordered_map>

#include "window.h"
#include "image.h"
#include "text.h"
#include "button.h"
#include "GUI.h"
#include "blackjack.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// Минимальный стартовый размер окна приложения (16:9 ориентация)
// Эти значения передаются в Run() как рекомендуемый размер окна
const int MIN_WIDTH  = 888;
const int MIN_HEIGHT = 500;

// Папка с PNG ассетами
static const char* ASSETS_DIR = "assets/";

// Простой кеш загруженных PNG (IMAGEINFO из CairoContext)
// Кеш позволяет избегать повторных LoadPNG при каждом обновлении экрана
static std::unordered_map<std::string, IMAGEINFO> g_imageCache;

static IMAGEINFO loadPNGCached(const std::string &path) {
    auto it = g_imageCache.find(path);
    if (it != g_imageCache.end()) return it->second;
    // theGUI — глобальный объект GtkPlus, реализующий Context::LoadPNG
    IMAGEINFO ii = theGUI->LoadPNG(path.c_str());
    g_imageCache[path] = ii;
    return ii;
}

static IMAGEINFO getBackPNG() {
    return loadPNGCached(std::string(ASSETS_DIR) + "back.png");
}

// Разбор Card::toString() ("A_hearts", "10_diamonds", "J_clubs", "K_spades")
// в имена файлов PNG вида "<rank>_of_<suit>.png"
static void parseCardToNames(const Card &card, std::string &rankName, std::string &suitName) {
    std::string s = card.toString(); // формат "<rank>_<suit>"
    size_t pos = s.find('_');
    if (pos == std::string::npos) {
        // Непредвиденный формат — помечаем "unknown"
        rankName = s;
        suitName = "unknown";
        return;
    }

    std::string r = s.substr(0, pos);
    std::string su = s.substr(pos + 1);

    // Допускаемые масти в названиях ассетов
    if (su == "hearts" || su == "diamonds" || su == "clubs" || su == "spades") {
        suitName = su;
    } else {
        suitName = "unknown";
    }

    // Приводим ранги к именам файлов
    if (r == "A")       rankName = "ace";
    else if (r == "J")  rankName = "jack";
    else if (r == "Q")  rankName = "queen";
    else if (r == "K")  rankName = "king";
    else                rankName = r; // "2".."10"
}

// Получить IMAGEINFO для карты через кеш и соответствие имени файла
static IMAGEINFO getCardPNG(const Card &card) {
    std::string rankName, suitName;
    parseCardToNames(card, rankName, suitName);
    std::string path = std::string(ASSETS_DIR) + rankName + "_of_" + suitName + ".png";
    return loadPNGCached(path);
}

// Вспомогательная логика подсчёта значения карты по её строковому представлению
static int cardValueForScore(const Card &c) {
    std::string s = c.toString();
    size_t pos = s.find('_');
    std::string rank = (pos == std::string::npos) ? s : s.substr(0, pos);

    if (rank == "A") return 11;
    if (rank == "J" || rank == "Q" || rank == "K") return 10;
    try { return std::stoi(rank); } catch(...) { return 0; }
}

// Подсчёт значения руки (тузы 11 или 1)
static int handValue(const std::vector<Card> &hand) {
    int sum = 0;
    int aces = 0;
    for (const auto &c : hand) {
        int v = cardValueForScore(c);
        if (v == 11) ++aces;
        sum += v;
    }
    while (sum > 21 && aces > 0) { sum -= 10; --aces; }
    return sum;
}

// Локальный GUI‑флаг: если true — интерфейс пометил игру как завершённую
static bool g_manualGameOver = false;

BlackjackWindow::BlackjackWindow() {
    m_ClassName = __FUNCTION__; // для диагностики/печати структуры окон
    SetBackColor(RGB(0.1, 0.5, 0.2)); // зелёный стол — задаём фон окна
}

void BlackjackWindow::OnCreate() {
    // Создаём текстовые надписи как дочерние окна Text
    // Text реализует OnDraw через Context::Text и управляет вычислением размеров
    m_dealerText = new Text("Дилер:");
    m_dealerText->SetFont(NULL, 20, -1, -1);
    m_dealerText->SetTextColor(RGB(1.0, 1.0, 1.0));
    m_dealerText->SetBackColor(GetBackColor());
    AddChild(m_dealerText, Point(0, 0), Rect(100, 30)); // Добавление в иерархию

    m_scoreText = new Text("");
    m_scoreText->SetFont(NULL, 18, -1, -1);
    m_scoreText->SetTextColor(RGB(1.0, 1.0, 1.0));
    m_scoreText->SetBackColor(GetBackColor());
    AddChild(m_scoreText, Point(0, 0), Rect(100, 30));

    m_playerText = new Text("Игрок:");
    m_playerText->SetFont(NULL, 20, -1, -1);
    m_playerText->SetTextColor(RGB(1.0, 1.0, 1.0));
    m_playerText->SetBackColor(GetBackColor());
    AddChild(m_playerText, Point(0, 0), Rect(100, 30));

    m_resultText = new Text("");
    m_resultText->SetFont(NULL, 18, -1, -1);
    m_resultText->SetTextColor(RGB(1.0, 1.0, 0.0));
    m_resultText->SetBackColor(GetBackColor());
    AddChild(m_resultText, Point(0, 0), Rect(100, 30));

    // Создаём кнопки управления — TextButton наследует Button, который наследует Window
    // При клике кнопки она вызывает NotifyParent с типом, равным m_command (CMD_*)
    m_hitButton = new TextButton("Взять", CMD_HIT);
    m_standButton = new TextButton("Хватит", CMD_STAND);
    m_newGameButton = new TextButton("Новая игра", CMD_NEW_GAME);

    m_hitButton->SetBackColor(RGB(0.2, 0.6, 0.2));
    m_standButton->SetBackColor(RGB(0.6, 0.2, 0.2));
    m_newGameButton->SetBackColor(RGB(0.2, 0.2, 0.6));

    // Добавляем кнопки как дочерние окна; реальные позиции/размеры выставляются в updateLayout()
    AddChild(m_hitButton, Point(0, 0), Rect(50, 20));
    AddChild(m_standButton, Point(0, 0), Rect(60, 20));
    AddChild(m_newGameButton, Point(0, 0), Rect(90, 20));

    // Лэйаут позиционирует дочерние окна. Он вызывается на старте и при изменении размера
    updateLayout();

    // Начинаем новую игру
    // После раздачи updateDisplay() покажет карты
    startNewGame();
}

void BlackjackWindow::OnSizeChanged() {
    // При изменении размера окна пересчитываем layout и обновляем отображение
    updateLayout();
    updateDisplay(); // перестроить дочерние картинки карт в соответствии с новой геометрией
}

void BlackjackWindow::updateLayout() {
    // Вычисляем расположение всех статичных элементов (надписи, кнопки)
    Point size = GetInteriorSize();

    // Для удобства раскладки используем виртуальную высоту по аспекту 16:9.
    int w = size.GetX();
    int h = (int)(w * 9.0 / 16.0);

    uint16_t margin       = std::max(8, (int)(w * 0.02));

    // Правый столбец отводится под кнопки; кнопки — дочерние окна TextButton
    uint16_t btnW         = w * 0.2;
    uint16_t btnH         = h * 0.08;
    uint16_t btnGap       = margin * 0.5;
    uint16_t btnX         = w - btnW - margin;
    uint16_t btnY         = h - (btnH * 3 + btnGap * 2) - margin;

    // Устанавливаем позиции/размеры кнопок через Window::SetPosition/SetSize
    m_hitButton->SetPosition(Point(btnX, btnY));
    m_hitButton->SetSize(Rect(btnW, btnH));
    m_standButton->SetPosition(Point(btnX, btnY + btnH + btnGap));
    m_standButton->SetSize(Rect(btnW, btnH));
    m_newGameButton->SetPosition(Point(btnX, btnY + (btnH + btnGap)*2));
    m_newGameButton->SetSize(Rect(btnW, btnH));

    // Левая колонка для карт и подписей (за исключением правой колонки с кнопками)
    uint16_t contentW     = btnX - margin;

    // Заголовки и зоны для дилера/игрока
    uint16_t titleH       = (uint16_t)std::max(14.0, h * 0.06);
    uint16_t rowGap       = (uint16_t)std::max(6.0, h * 0.015);

    uint16_t dealerTitleY = margin;
    uint16_t dealerZoneY  = dealerTitleY + titleH + rowGap;
    uint16_t dealerZoneH  = (uint16_t)(h * 0.36);

    uint16_t scoreTitleY  = dealerZoneY + dealerZoneH + rowGap;
    uint16_t playerTitleY = scoreTitleY + titleH + rowGap;
    uint16_t playerZoneY  = playerTitleY + titleH + rowGap;
    uint16_t playerZoneH  = (uint16_t)(h * 0.36);

    // Устанавливаем позицию/размер текстовых надписей
    m_dealerText->SetPosition(Point(margin, dealerTitleY));
    m_dealerText->SetSize(Rect(contentW, titleH));

    m_scoreText->SetPosition(Point(margin, scoreTitleY));
    m_scoreText->SetSize(Rect(contentW, titleH));

    m_playerText->SetPosition(Point(margin, playerTitleY));
    m_playerText->SetSize(Rect(contentW, titleH));

    // Результат игры располагается в правом верхнем углу
    uint16_t resultW = (uint16_t)std::min((int)(w * 0.17), 360);
    m_resultText->SetPosition(Point(w - margin - resultW, margin));
    m_resultText->SetSize(Rect(resultW, titleH));
}

void BlackjackWindow::OnDraw(Context *cr) {
    // Рендер фона окна — вызывается при каждой перерисовке
    Point size = GetInteriorSize();
    cr->SetColor(GetBackColor());
    cr->FillRectangle(Point(0,0), size);
}

bool BlackjackWindow::OnKeyPress(uint64_t keyval) {
    // Клавиши управления: q - выход, n - новая игра, h - hit, s - stand
    // Если GUI локально пометил игру как завершённую, игнорируем всё, кроме 'n'
    if (g_manualGameOver && keyval != 'n') return true;

    switch(keyval) {
        case 'q':
            DeleteMe(); // запрос удаления окна у родителя
            return true;
        case 'n':
            startNewGame();
            return true;
        case 'h':
            game.playerHit();
            {
                int ps = game.getPlayerScore();
                // Если игрок достиг 21 или перебора — помечаем GUI как завершённый и отображаем результат
                if (ps == 21 || ps > 21) {
                    g_manualGameOver = true;
                    m_resultText->SetText(game.getGameResult().c_str());
                    updateDisplay();
                    ReDraw();
                    return true;
                }
            }
            updateDisplay();
            return true;
        case 's':
            game.playerStand();
            updateDisplay();
            if (game.getGameState() == GAME_OVER) {
                std::string res = game.getGameResult();
                if (res.empty()) {
                    int ds = game.getDealerScore();
                    int ps = game.getPlayerScore();
                    if (ds > 21 && ps <= 21) res = " Вы победили!";
                    else if (ps > 21 && ds <= 21) res = "Вы проиграли!";
                    else if (ps > ds) res = " Вы победили!";
                    else if (ps < ds) res = "Вы проиграли!";
                    else res = "    Ничья";
                }
                g_manualGameOver = true;
                m_resultText->SetText(res.c_str());
                ReDraw();
            }
            return true;
        default:
            return true;
    }
}

void BlackjackWindow::OnNotify(Window *child, uint32_t type, const Point &position) {
    // Здесь обрабатываем команды CMD_HIT, CMD_STAND, CMD_NEW_GAME
    if (g_manualGameOver && type != CMD_NEW_GAME) return;

    switch(type) {
        case CMD_HIT:
            game.playerHit();

            // Проверка состояния игрока сразу после хита; при 21 или переборе — отмечаем конец игры
            {
                int ps = game.getPlayerScore();
                if (ps == 21 || ps > 21) {
                    g_manualGameOver = true;
                    m_resultText->SetText(game.getGameResult().c_str());
                    updateDisplay();
                    ReDraw();
                    break;
                }
            }
            updateDisplay();
            break;

        case CMD_STAND:
            game.playerStand();
            updateDisplay();

            if (game.getGameState() == GAME_OVER) {
                std::string res = game.getGameResult();
                if (res.empty()) {
                    int ds = game.getDealerScore();
                    int ps = game.getPlayerScore();
                    if (ds > 21 && ps <= 21) res = " Вы победили!";
                    else if (ps > 21 && ds <= 21) res = "Вы проиграли!";
                    else if (ps > ds) res = " Вы победили!";
                    else if (ps < ds) res = "Вы проиграли!";
                    else res = "     Ничья";
                }
                g_manualGameOver = true;
                m_resultText->SetText(res.c_str());
                ReDraw();
            }

            break;

        case CMD_NEW_GAME:
            startNewGame();
            break;
    }
}

void BlackjackWindow::startNewGame() {
    // Сбрасываем GUI флаг окончания и текст результата
    g_manualGameOver = false;
    m_resultText->SetText("");

    game.dealInitialCards();

    // Если у игрока натуральный 21 при раздаче — сразу показываем результат
    int ps = game.getPlayerScore();
    if (ps == 21) {
        g_manualGameOver = true;
        Point sz = GetInteriorSize();
        double scale = (double)sz.GetX() / 888.0;
        m_resultText->SetFont(NULL, (uint16_t)std::max(14.0, 16.0 * scale), -1, -1);

        m_resultText->SetTextColor(RGB(1.0, 1.0, 0.0));
        m_resultText->SetBackColor(GetBackColor());
        m_resultText->SetText(game.getGameResult().c_str());
        updateDisplay();
        ReDraw();
        return;
    }

    updateDisplay();
}

void BlackjackWindow::updateDisplay() {
    bool hideDealerCard = (game.getGameState() == PLAYER_TURN);

    // Счёт отдельной строкой не используем; показываем очки рядом с подписью
    m_scoreText->SetText("");

    // Обновляем текстовые надписи дилера/игрока Text::SetText -> перерисовка дочернего окна
    if (hideDealerCard) {
        const auto &dealer = game.getDealerHand();
        int visibleSum = 0;
        if (dealer.size() > 1) {
            std::vector<Card> visibleCards(dealer.begin() + 1, dealer.end());
            visibleSum = handValue(visibleCards);
        }
        std::string dealerLabel = "Дилер: " + std::to_string(visibleSum) + " + ?";
        m_dealerText->SetText(dealerLabel.c_str());
    } else {
        std::string dealerLabel = "Дилер: " + std::to_string(game.getDealerScore());
        m_dealerText->SetText(dealerLabel.c_str());
    }
    std::string playerLabel = "Игрок: " + std::to_string(game.getPlayerScore());
    m_playerText->SetText(playerLabel.c_str());

    // Обновляем поле результата в зависимости от состояния
    if (g_manualGameOver) {
        // Если GUI пометил игру как завершённую, не затираем текст результата
        Point sz = GetInteriorSize();
        double scale = (double)sz.GetX() / 888.0;
        m_resultText->SetFont(NULL, (uint16_t)std::max(14.0, 16.0 * scale), -1, -1);
        m_resultText->SetTextColor(RGB(1.0, 1.0, 0.0));
        m_resultText->SetBackColor(GetBackColor());
    } else if (game.getGameState() == GAME_OVER) {
        std::string res = game.getGameResult();
        if (res.empty()) {
            int ds = game.getDealerScore();
            int ps = game.getPlayerScore();
            if (ds > 21 && ps <= 21) res = " Вы победили!";
            else if (ps > 21 && ds <= 21) res = "Вы проиграли!";
            else if (ps > ds) res = " Вы победили!";
            else if (ps < ds) res = "Вы проиграли!";
            else res = "    Ничья";
        }
        m_resultText->SetText(res.c_str());
    } else {
        m_resultText->SetText("");
    }

    // Удаляем предыдущие карты, которые были добавлены как дочерние окна
    static std::vector<Image*> s_dealerImgs;
    static std::vector<Image*> s_playerImgs;
    for (auto *img : s_dealerImgs) { if (img) DeleteChild(img); }
    for (auto *img : s_playerImgs) { if (img) DeleteChild(img); }
    s_dealerImgs.clear();
    s_playerImgs.clear();

    // Пересчитываем геометрию зон аналогично updateLayout
    Point size = GetInteriorSize();
    int w = size.GetX();
    int h = (int)(w * 9.0 / 16.0);
    if (w < 480) w = 480;
    if (h < 270) h = 270;

    uint16_t margin       = std::max(8, (int)(w * 0.02));
    uint16_t btnW         = w * 0.16;
    uint16_t btnX         = w - btnW - margin;
    uint16_t contentW     = btnX - margin;
    uint16_t titleH       = (uint16_t)std::max(14.0, h * 0.06);
    uint16_t rowGap       = (uint16_t)std::max(6.0, h * 0.015);

    uint16_t dealerTitleY = margin;
    uint16_t dealerZoneY  = dealerTitleY + titleH + rowGap;
    uint16_t dealerZoneH  = (uint16_t)(h * 0.36);

    uint16_t scoreTitleY  = dealerZoneY + dealerZoneH + rowGap;
    uint16_t playerTitleY = scoreTitleY + titleH + rowGap;
    uint16_t playerZoneY  = playerTitleY + titleH + rowGap;
    uint16_t playerZoneH  = (uint16_t)(h * 0.36);

    // Параметры карт: рассчитываем размеры, соотношение сторон, отступы
    double   cardAspect   = 0.7; // примерное W/H для карт
    uint16_t cardH        = (uint16_t)std::min<double>(dealerZoneH * 0.92, h * 0.55);
    uint16_t cardW        = (uint16_t)(cardH * cardAspect);
    uint16_t cardGapX     = std::max((int)(w * 0.01), (int)(cardW * 0.06));
    uint16_t cardGapY     = std::max((int)(h * 0.02), (int)(cardH * 0.06));

    uint16_t laneX        = margin;
    uint16_t laneW        = contentW;

    // 1) Отрисовка карт дилера
    const auto &dealer = game.getDealerHand();
    {
        uint16_t x = laneX;
        uint16_t y = dealerZoneY;
        uint16_t rowH = cardH;

        if (hideDealerCard && dealer.size() >= 2) {
            // Рисуем видимые карты дилера (с индекса 1), затем рубашку в следующей позиции
            for (size_t i = 1; i < dealer.size(); ++i) {
                if (x + cardW > laneX + laneW) {
                    x = laneX;
                    y += rowH + cardGapY;
                    if (y + cardH > dealerZoneY + dealerZoneH) break;
                }
                IMAGEINFO ii = getCardPNG(dealer[i]);
                if (ii) {
                    Image *img = new Image(ii);      // Image — дочернее окно, которое нарисует PNG
                    img->SetStyle(IMAGE_SCALE_FIT);  // IMAGE_SCALE_FIT — масштабирование с сохранением пропорций
                    AddChild(img, Point(x, y), Rect(cardW, cardH));
                    s_dealerImgs.push_back(img);
                }
                x += cardW + cardGapX;
            }

            // Рисуем рубашку после видимых карт; если нет места — перенос на следующую строку
            if (y + cardH <= dealerZoneY + dealerZoneH) {
                if (x + cardW > laneX + laneW) {
                    x = laneX;
                    y += rowH + cardGapY;
                }
                if (y + cardH <= dealerZoneY + dealerZoneH) {
                    IMAGEINFO back = getBackPNG();
                    if (back) {
                        Image *img = new Image(back);
                        img->SetStyle(IMAGE_SCALE_FIT);
                        AddChild(img, Point(x, y), Rect(cardW, cardH));
                        s_dealerImgs.push_back(img);
                    }
                }
            }
        } else {
            // Обычная отрисовка всех карт дилера (если не скрываем первую карту)
            x = laneX;
            y = dealerZoneY;
            for (size_t i = 0; i < dealer.size(); ++i) {
                IMAGEINFO ii = (i == 0 && hideDealerCard) ? getBackPNG() : getCardPNG(dealer[i]);
                if (x + cardW > laneX + laneW) {
                    x = laneX;
                    y += rowH + cardGapY;
                    if (y + cardH > dealerZoneY + dealerZoneH) break;
                }
                if (ii) {
                    Image *img = new Image(ii);
                    img->SetStyle(IMAGE_SCALE_FIT);
                    AddChild(img, Point(x, y), Rect(cardW, cardH));
                    s_dealerImgs.push_back(img);
                }
                x += cardW + cardGapX;
            }
        }
    }

    // 2) Отрисовка карт игрока — слева направо, с переносом строк
    const auto &player = game.getPlayerHand();
    {
        uint16_t x = laneX;
        uint16_t y = playerZoneY;
        uint16_t rowH = cardH;
        for (size_t i = 0; i < player.size(); ++i) {
            IMAGEINFO ii = getCardPNG(player[i]);

            if (x + cardW > laneX + laneW) {
                x = laneX;
                y += rowH + cardGapY;
                if (y + cardH > playerZoneY + playerZoneH) break;
            }

            if (ii) {
                Image *img = new Image(ii);
                img->SetStyle(IMAGE_SCALE_FIT);
                AddChild(img, Point(x, y), Rect(cardW, cardH));
                s_playerImgs.push_back(img);
            }
            x += cardW + cardGapX;
        }
    }
    ReDraw();
}

int main(int argc, char **argv) {
    BlackjackWindow *window = new BlackjackWindow();
    int res = Run(argc, argv, window, MIN_WIDTH, MIN_HEIGHT);
    delete window;
    return res;
}

