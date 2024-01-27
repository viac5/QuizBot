#include <stdio.h>
#include <tgbot/tgbot.h>
#include <iostream>
#include <libpq-fe.h>
#include <string>
#include <vector>
#include <sstream>
#include <random>

using namespace TgBot;
using namespace std;
PGconn* conn = PQconnectdb("dbname=QuizBot host=localhost user=postgres password=******");


// Функция для отправки сообщения в Телеграм
void sendMessage(TgBot::Bot& bot, int64_t chatId, const std::string& text) {
    bot.getApi().sendMessage(chatId, text);
}

enum class QuestionState {// Переменная состояния
    NONE,
    NAME,
    TEXT
};

void onAddCommand(Bot& bot, const Message::Ptr& message, QuestionState& state) {
    if (state == QuestionState::NONE) {
        // Запрашиваем имя вопроса
        sendMessage(bot, message->chat->id, "Write a name of question ");
        state = QuestionState::NAME;
    }
}

void onAnyMessage(Bot& bot, const Message::Ptr& message, QuestionState& state, std::string& nameQuestion, PGconn* conn) {
    if (state == QuestionState::NAME) {
        nameQuestion = message->text;
        // Запрашиваем текст вопроса
        sendMessage(bot, message->chat->id, "Write a text of question ");
        state = QuestionState::TEXT;
    }
    else if (state == QuestionState::TEXT) {
        std::string infoInQuestion = message->text;
        PGresult* res;
        std::string username = message->from->firstName;
        std::string queryStr = "INSERT INTO inf_for_learn(user_id, username, name_of_question, main_data) VALUES ('" + std::to_string(message->chat->id) + "','" + username + "','" + nameQuestion + "','" + infoInQuestion + "'); ";
        const char* query = queryStr.c_str();
        res = PQexec(conn, query);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            sendMessage(bot, message->chat->id, "Failed to save data: ");
        }
        else {
            sendMessage(bot, message->chat->id, "Data saved successfully!");
        }
        PQclear(res);

        // Reset the state
        state = QuestionState::NONE;
    }
}
void forCheckTable(Bot& bot, const Message::Ptr& message) {
    std::string query = "SELECT name_of_question, main_data FROM inf_for_learn WHERE user_id = '" + std::to_string(message->chat->id) + "';";
    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        std::string response;
        for (int i = 0; i < PQntuples(res); i++) {
            response += std::to_string(i + 1) + ". " + std::string(PQgetvalue(res, i, 0)) + ": " + std::string(PQgetvalue(res, i, 1)) + "\n";
            bot.getApi().sendMessage(message->chat->id, response);
            response = "";
        }

    }
    else {
        bot.getApi().sendMessage(message->chat->id, PQresultErrorMessage(res));
    }

    PQclear(res);
}
enum class DeleteState {// Переменная состояния
    NONE,
    NAME  

};
void CommandDelete(Bot& bot, const Message::Ptr& message, DeleteState& state1) {
    if (state1 == DeleteState::NONE) {
        // Запрашиваем имя вопроса
        sendMessage(bot, message->chat->id, "Write a name of question which you want to delete ");
        state1 = DeleteState::NAME;
    }
}
void AnyDelete(Bot& bot, const Message::Ptr& message, DeleteState& state1, std::string& nameDelQuestion, PGconn* conn) {
    PGresult* res;
    if (state1 == DeleteState::NAME) {
        nameDelQuestion = message->text;
        std::string queryStr = "DELETE FROM inf_for_learn WHERE name_of_question = '" + nameDelQuestion + "'; ";
        const char* query = queryStr.c_str();
        res = PQexec(conn, query);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            sendMessage(bot, message->chat->id, "Failed to delete data: ");
        }
        else {
            sendMessage(bot, message->chat->id, "Data delete successfully!");
        }
        PQclear(res);

        // Reset the state
        state1 = DeleteState::NONE;

    }
    
}

void LearnQuestions(Bot& bot, const Message::Ptr& message) {
    std::string query = "SELECT name_of_question, main_data FROM inf_for_learn WHERE user_id = '" + std::to_string(message->chat->id) + "';";
    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        std::string NameQuetionsForLearn;
        std::vector<std::string> Quetions;  // Инициализация вектора строк
        std::vector<int> NumbersOfQuetons;
        for (int i = 0; i < PQntuples(res); i++) {
            NameQuetionsForLearn += std::to_string(i + 1) + ". " + std::string(PQgetvalue(res, i, 0));
            Quetions.push_back(NameQuetionsForLearn);
            NumbersOfQuetons.push_back(i);
            NameQuetionsForLearn = "";
        }
        std::random_device rd;  // Инициализация генератора случайных чисел
        std::mt19937 gen(rd());  // Использование генератора случайных чисел Mersenne Twister
        std::shuffle(NumbersOfQuetons.begin(), NumbersOfQuetons.end(), gen);
        for (int j = 0; j < PQntuples(res); j++)
        {
            int num = NumbersOfQuetons[j];
            sendMessage(bot, message->chat->id, Quetions[num]);
        }

    }
    else {
        bot.getApi().sendMessage(message->chat->id, PQresultErrorMessage(res));
    }

    PQclear(res);
}


int main() {
    Bot bot("token");


    // Установка соединения с базой данных
    PGconn* conn = PQconnectdb("dbname=QuizBot host=localhost user=postgres password=3141592");

    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn);
        PQfinish(conn);
        return 1;
    }
    //PQsetClientEncoding(conn, "UTF8");

    bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hellow!");
        });


    QuestionState state = QuestionState::NONE; // устанавливаем значение переменной состояния NONE
    std::string nameQuestion;

    bot.getEvents().onCommand("add", [&bot, &state](Message::Ptr message) {
        onAddCommand(bot, message, state);
        });
    bot.getEvents().onAnyMessage([&bot, &state, &nameQuestion, &conn](Message::Ptr message) {
        onAnyMessage(bot, message, state, nameQuestion, conn);
        });
    bot.getEvents().onCommand("check", [&bot, conn](Message::Ptr message) {
        forCheckTable(bot, message);
        });

    DeleteState state1 = DeleteState::NONE; // устанавливаем значение переменной состояния NONE
    std::string nameDelQuestion;

    bot.getEvents().onCommand("delete", [&bot, &state1](Message::Ptr message) {
        CommandDelete(bot, message, state1);
        });
    bot.getEvents().onAnyMessage([&bot, &state1, &nameDelQuestion, &conn](Message::Ptr message) {
        AnyDelete(bot, message, state1, nameDelQuestion, conn);
        });
    bot.getEvents().onCommand("learn", [&bot, &state1](Message::Ptr message) {
        LearnQuestions(bot, message);
        });




    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgException& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}

