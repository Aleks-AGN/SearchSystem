#include "string_processing.h"

//using namespace std;

/*std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}*/

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    // Удаляем начало из text до первого непробельного символа
    text.remove_prefix(std::min(text.find_first_not_of(" "), text.size()));

    // Используем find с одним параметром чтобы найти номер позиции первого пробела
    while(text != text.end()) {
        size_t space = text.find(' '); 
        // Добавляем в result элемент string_view, полученный вызовом метода substr, 
        // где начальная позиция будет 0, а конечная — найденная позиция пробела или npos.
        result.push_back(space == text.npos ? text.substr(0, text.npos) : text.substr(0, space));
        // Сдвигаем начало text так, чтобы оно указывало на позицию за пробелом,
        // передвигая начало text на указанное в аргументе количество позиций
        text.remove_prefix(std::min(text.find_first_not_of(" ", space), text.size()));
    }
    return result;
}
