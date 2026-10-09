#include "NineMensMorris.h"
#include "Minimax.h"

#include <QApplication>
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QMessageBox>
#include <QTimer>
#include <QWidget>
#include <algorithm>
#include <array>
#include <cmath>

class MillsWindow final : public QWidget
{
public:
    MillsWindow()
    {
        setWindowTitle("Мельница");
        resize(1120, 760);
        setMinimumSize(900, 650);
        game.reset();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#f5f3ed"));
        const int side = std::min(height() - 90, width() - 440);
        const int left = 52, top = (height() - side) / 2;
        const int unit = side / 6;
        auto pt = [&](int i) { return QPoint(left + coords[i][0] * unit, top + coords[i][1] * unit); };

        p.setPen(QPen(QColor("#a69c8c"), 3, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 24; ++i) for (int j : GameBoard::neighbours(i))
            if (j > i) p.drawLine(pt(i), pt(j));

        p.setPen(Qt::NoPen);
        const auto moves = game.legalMoves();
        for (int i = 0; i < 24; ++i) {
            const QPoint c = pt(i);
            const bool legal = std::any_of(moves.begin(), moves.end(), [i](const Move& m){ return m.to == i; });
            QColor fill = legal && !game.isGameOver() ? QColor("#c8d7c4") : QColor("#e7e1d7");
            if (i == selectedFrom || (captureMode && isCaptureTarget(i))) fill = QColor("#e8b06c");
            p.setBrush(fill); p.drawEllipse(c, 16, 16);
            const Cell cell = game.cell(i);
            if (cell != Cell::Empty) {
                p.setBrush(cell == Cell::Player0 ? QColor("#334a42") : QColor("#c86b4a"));
                p.setPen(QPen(QColor("#fffaf0"), 3)); p.drawEllipse(c, 12, 12); p.setPen(Qt::NoPen);
            }
        }

        const int panelX = left + side + 48;
        p.setPen(QColor("#25332d")); p.setFont(QFont("Georgia", 24, QFont::DemiBold));
        p.drawText(panelX, top + 38, "Мельница"); p.drawText(panelX, top + 68, "ДЕВЯТЬ ФИШЕК");
        p.setFont(QFont("Arial", 10)); p.setPen(QColor("#7f786e")); p.drawText(panelX, top + 99, "С БОТОМ ИЛИ РЕАЛЬНЫМ СОПЕРНИКОМ");
        p.setPen(QColor("#25332d")); p.setFont(QFont("Arial", 12, QFont::DemiBold));
        QString status;
        if (game.isGameOver()) status = game.winner() == Player::Player0 ? "Победили зелёные" : "Победили терракотовые";
        else if (botThinking) status = "Бот думает…";
        else if (captureMode) status = "Снимите фишку соперника";
        else if (mode == GameMode::AgainstBot && game.currentPlayer() == Player::Player1) status = "Ход бота";
        else status = game.currentPlayer() == Player::Player0 ? "Ход зелёных" : "Ход терракотовых";
        p.drawText(panelX, top + 151, status);
        p.setFont(QFont("Arial", 10)); p.setPen(QColor("#7f786e")); p.drawText(panelX, top + 178, modeText() + "  ·  " + phaseText());
        drawPlayer(p, panelX, top + 229, Player::Player0, "ЗЕЛЁНЫЕ", QColor("#334a42"));
        drawPlayer(p, panelX, top + 333, Player::Player1,
                   mode == GameMode::AgainstBot ? "БОТ · ТЕРРАКОТОВЫЕ" : "ТЕРРАКОТОВЫЕ", QColor("#c86b4a"));
        p.setPen(QColor("#d3ccc0")); p.drawLine(panelX, top + 405, width() - 42, top + 405);
        p.setPen(QColor("#25332d")); p.setFont(QFont("Arial", 10, QFont::DemiBold)); p.drawText(panelX, top + 432, "РЕЖИМ ИГРЫ");
        drawModeButton(p, panelX, top + 442, 126, "С БОТОМ", mode == GameMode::AgainstBot);
        drawModeButton(p, panelX + 138, top + 442, 126, "ВДВОЁМ", mode == GameMode::TwoPlayers);
        p.setPen(QColor("#25332d")); p.setFont(QFont("Arial", 11, QFont::DemiBold)); p.drawText(panelX, top + 492, "КРАТКО О ПРАВИЛАХ");
        p.setFont(QFont("Arial", 10)); p.setPen(QColor("#716b62"));
        p.drawText(QRect(panelX, top + 513, width() - panelX - 36, 36), Qt::TextWordWrap,
                   "Выставьте по девять фишек. Соберите три в ряд, чтобы снять фишку соперника. Затем передвигайте фишки по линиям.");
        p.setPen(QColor("#25332d")); p.setBrush(QColor("#25332d")); p.drawRoundedRect(panelX, top + side - 4, 126, 38, 8, 8);
        p.setPen(QColor("#fffaf0")); p.setFont(QFont("Arial", 10, QFont::DemiBold)); p.drawText(panelX + 20, top + side + 20, "НОВАЯ ИГРА");
        p.setPen(QColor("#25332d")); p.setBrush(QColor("#25332d")); p.drawRoundedRect(panelX + 138, top + side - 4, 112, 38, 8, 8);
        p.setPen(QColor("#fffaf0")); p.drawText(panelX + 164, top + side + 20, "ПРАВИЛА");
        p.setPen(QColor("#b3aa9c")); p.setFont(QFont("Arial", 9)); p.drawText(52, height() - 18, "МЕЛЬНИЦА  /  01");
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        const QPoint pos = event->position().toPoint();
        const int side = std::min(height() - 90, width() - 440), left = 52, top = (height() - side) / 2;
        const int panelX = left + side + 48;
        if (pos.x() >= panelX + 138 && pos.x() <= panelX + 250 && pos.y() >= top + side - 4 && pos.y() <= top + side + 34) {
            QMessageBox::information(this, "Правила игры — Мельница",
                "Цель игры\nОставить соперника менее чем с тремя фишками или лишить его возможности ходить.\n\n"
                "1. Выставление фишек. Игроки по очереди ставят по одной фишке на свободные точки доски. Всего у каждого по девять фишек.\n\n"
                "2. Мельница. Три фишки одного цвета, стоящие подряд на одной линии, образуют мельницу. Собрав её, игрок снимает одну фишку соперника. Обычно нельзя снимать фишку из его мельницы, пока у него есть фишки вне мельниц.\n\n"
                "3. Передвижение. Когда все фишки выставлены, за ход передвиньте одну свою фишку на соседнюю свободную точку по линии. Если у игрока осталось три фишки, он может переставить фишку на любую свободную точку.\n\n"
                "Победа. Вы выигрываете, если у соперника осталось меньше трёх фишек или он не может сделать ход.");
            return;
        }
        if (pos.x() >= panelX && pos.x() <= panelX + 126 && pos.y() >= top + side - 4 && pos.y() <= top + side + 34) {
            startNewGame(); return;
        }
        if (pos.x() >= panelX && pos.x() <= panelX + 126 && pos.y() >= top + 442 && pos.y() <= top + 480) {
            setMode(GameMode::AgainstBot); return;
        }
        if (pos.x() >= panelX + 138 && pos.x() <= panelX + 264 && pos.y() >= top + 442 && pos.y() <= top + 480) {
            setMode(GameMode::TwoPlayers); return;
        }
        const int unit = side / 6;
        int hit = -1; double best = 25.0 * 25.0;
        for (int i = 0; i < 24; ++i) {
            const QPoint c(left + coords[i][0] * unit, top + coords[i][1] * unit);
            const double d = std::pow(pos.x() - c.x(), 2) + std::pow(pos.y() - c.y(), 2);
            if (d < best) best = d, hit = i;
        }
        if (hit < 0 || game.isGameOver() || botThinking || isBotTurn()) return;
        const auto moves = game.legalMoves();
        if (captureMode) {
            const auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m){ return m.from == pendingFrom && m.to == pendingTo && m.capture == hit; });
            if (it != moves.end()) { game.makeMove(*it); selectedFrom = -1; captureMode = false; scheduleBotMove(); }
            update(); return;
        }
        const auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& m){ return m.to == hit && (m.from == -1 || m.from == selectedFrom); });
        if (it != moves.end()) {
            if (it->capture >= 0) { captureMode = true; pendingFrom = it->from; pendingTo = it->to; selectedFrom = -1; }
            else { game.makeMove(*it); selectedFrom = -1; scheduleBotMove(); }
            update(); return;
        }
        if (game.cell(hit) == (game.currentPlayer() == Player::Player0 ? Cell::Player0 : Cell::Player1) && game.piecesInHand(game.currentPlayer()) == 0) selectedFrom = hit;
        else selectedFrom = -1;
        update();
    }

private:
    enum class GameMode { AgainstBot, TwoPlayers };

    static constexpr std::array<std::array<int, 2>, 24> coords = {{
        {{0,0}},{{3,0}},{{6,0}},{{6,3}},{{6,6}},{{3,6}},{{0,6}},{{0,3}},
        {{1,1}},{{3,1}},{{5,1}},{{5,3}},{{5,5}},{{3,5}},{{1,5}},{{1,3}},
        {{2,2}},{{3,2}},{{4,2}},{{4,3}},{{4,4}},{{3,4}},{{2,4}},{{2,3}}
    }};
    NineMensMorris game;
    GameBot bot{3};
    GameMode mode = GameMode::AgainstBot;
    int selectedFrom = -1, pendingFrom = -1, pendingTo = -1;
    bool captureMode = false;
    bool botThinking = false;

    bool isCaptureTarget(int pos) const {
        const auto moves = game.legalMoves();
        return std::any_of(moves.begin(), moves.end(), [&](const Move& m){ return m.from == pendingFrom && m.to == pendingTo && m.capture == pos; });
    }
    QString phaseText() const { return game.piecesInHand(game.currentPlayer()) > 0 ? "ЭТАП 1  /  ВЫСТАВЛЕНИЕ ФИШЕК" : "ЭТАП 2  /  ПЕРЕДВИЖЕНИЕ ФИШЕК"; }
    QString modeText() const { return mode == GameMode::AgainstBot ? "ИГРА С БОТОМ" : "ИГРА ВДВОЁМ"; }
    bool isBotTurn() const { return mode == GameMode::AgainstBot && game.currentPlayer() == Player::Player1; }

    void startNewGame()
    {
        game.reset();
        selectedFrom = pendingFrom = pendingTo = -1;
        captureMode = false;
        botThinking = false;
        update();
    }

    void setMode(GameMode newMode)
    {
        if (mode == newMode)
            return;

        mode = newMode;
        startNewGame();
    }

    void scheduleBotMove()
    {
        if (!isBotTurn() || game.isGameOver())
            return;

        botThinking = true;
        update();
        QTimer::singleShot(120, this, [this] {
            if (!isBotTurn() || game.isGameOver())
                return;

            if (const auto move = bot.findBestMove(game))
                game.makeMove(*move);

            botThinking = false;
            update();
        });
    }

    void drawModeButton(QPainter& p, int x, int y, int width, const QString& text, bool active) const
    {
        p.setPen(active ? QColor("#25332d") : QColor("#b3aa9c"));
        p.setBrush(active ? QColor("#25332d") : QColor("#f5f3ed"));
        p.drawRoundedRect(x, y, width, 38, 8, 8);
        p.setPen(active ? QColor("#fffaf0") : QColor("#25332d"));
        p.setFont(QFont("Arial", 9, QFont::DemiBold));
        p.drawText(QRect(x, y, width, 38), Qt::AlignCenter, text);
    }
    void drawPlayer(QPainter& p, int x, int y, Player player, const QString& label, const QColor& color) {
        p.setPen(Qt::NoPen); p.setBrush(color); p.drawEllipse(QPoint(x + 8, y - 4), 7, 7);
        p.setPen(QColor("#7f786e")); p.setFont(QFont("Arial", 9, QFont::DemiBold)); p.drawText(x + 24, y, label);
        p.setPen(QColor("#25332d")); p.setFont(QFont("Georgia", 29)); p.drawText(x, y + 42, QString::number(game.piecesOnBoard(player)));
        p.setPen(QColor("#8f887e")); p.setFont(QFont("Arial", 10)); p.drawText(x + 29, y + 40, "/ 9 НА ДОСКЕ");
        const int hand = game.piecesInHand(player);
        for (int i = 0; i < 9; ++i) { p.setPen(QPen(QColor("#d6cec2"), 1)); p.setBrush(i < hand ? color : QColor("#e7e1d7")); p.drawEllipse(x + i * 18, y + 57, 10, 10); }
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MillsWindow window;
    window.show();
    return app.exec();
}
