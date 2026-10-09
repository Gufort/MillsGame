#pragma once

#include "NineMensMorris.h"
#include <algorithm>
#include <limits.h>
#include <limits>
#include <vector>

class GameBot
{
public:
    explicit GameBot(int searchDepth = 4) : _searchDepth(searchDepth) {};

    std::optional<Move> findBestMove(NineMensMorris& game)
    {
        const auto moves = game.legalMoves();

        if (moves.empty())
            return std::nullopt;

        std::optional<Move> bestMove;
        int bestScore = -INT_MAX;
        int alpha = -INT_MAX;
        int beta = INT_MAX;

        for (const auto& move : moves)
        {
            if (!game.makeMove(move))
                continue;

            const int score = -negamax(game, _searchDepth - 1, -beta, -alpha, 1);

            game.undo();

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move;
            }

            alpha = std::max(alpha, score);
        }

        return bestMove;
    }

private:
    int evaluate(const NineMensMorris& game) const
    {
        const Player player = game.currentPlayer();
        const Player enemy = opponent(player);

        int score = 0;

        // Материальное преимущество - количество фишек на доске
        score += 100 * (
            game.piecesOnBoard(player) -
            game.piecesOnBoard(enemy)
        );

        // Фишки, которые ещё находятся в руке
        score += 80 * (
            game.piecesInHand(player) -
            game.piecesInHand(enemy)
        );

        const auto& state = game.getGameState();

        const Cell playerCell = cellForPlayer(player);
        const Cell enemyCell = cellForPlayer(enemy);

        for (const auto& mill: GameBoard::getMills())
        {
            int ownPieces = 0;
            int enemyPieces = 0;
            int emptyCells = 0;

            for (const auto position: mill)
            {
                auto cell = state.board[position];

                if (cell == playerCell) ownPieces++;
                else if (cell == enemyCell) enemyPieces++;
                else emptyCells++;
            }


            // Уже сформированные нами мельницы
            if (ownPieces == 3)
                score += 30;


            // Уже сформированные противником мельницы
            if (enemyPieces == 3)
                score -= 30;

            // Один шаг до мельницы
            if (ownPieces == 2 && emptyCells == 1)
                score += 10;

            if (enemyPieces == 2 && emptyCells == 1)
                score -= 10;
        }
        return score;
    }
    int negamax(NineMensMorris& game, int depth, int alpha, int beta, int ply)
    {
        if (game.isGameOver())
            return -WinScore + ply;

        if (depth <= 0)
            return evaluate(game);

        auto moves = game.legalMoves();

        int bestScore = -INT_MAX;

        for (const auto& move : moves)
        {
            if (!game.makeMove(move))
                continue;

            // Рекурсивно оцениваем ответ противника.
            const int score = -negamax(game,depth - 1,-beta,-alpha,ply + 1);

            // Отменяем ход, чтобы исследовать следующую ветвь из той же позиции
            game.undo();

            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, score);

            if (alpha >= beta)
                break;
        }
        return bestScore;
    }

    static Cell cellForPlayer(Player player)
    {
        return player == Player::Player0 ? Cell::Player0 : Cell::Player1;
    }

private:
    static constexpr int WinScore = 100'000;
    int _searchDepth;
};