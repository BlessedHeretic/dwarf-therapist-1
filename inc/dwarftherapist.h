/*
Dwarf Therapist
Copyright (c) 2009 Trey Stout (chmod)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef DWARF_THERAPIST_H
#define DWARF_THERAPIST_H

#define DT (static_cast<DwarfTherapist *>(QCoreApplication::instance()))

#include <QApplication>
#include <QVector>
#include "defines.h"

class QListWidgetItem;
class MainWindow;
class OptionsMenu;
class QSettings;
class CustomProfession;
class Dwarf;
class Word;
class DFInstance;
class LogManager;

class DwarfTherapist : public QApplication {
    Q_OBJECT
public:
    DwarfTherapist(int &argc, char **argv);
    virtual ~DwarfTherapist(){}

    QVector<CustomProfession*> get_custom_professions() {return m_custom_professions;}
    CustomProfession *get_custom_profession(QString name);
    MainWindow *get_main_window() {return m_main_window;}

    int custom_profession_from_dwarf(Dwarf *d);

    QSettings *user_settings() {return m_user_settings;}
    OptionsMenu *get_options_menu() {return m_options_menu;}
    Dwarf *get_dwarf_by_id(int dwarf_id);

    void load_game_translation_tables(DFInstance *df);
    QString get_generic_word(const uint &offset) {return m_generic_words.value(offset, "UNKNOWN");}
    QString get_dwarf_word(const uint &offset) {return m_dwarf_words.value(offset, get_generic_word(offset));}
    Word * get_word(const uint & offset) { return m_language.value(offset, NULL); }
    bool labor_cheats_allowed() {return m_allow_labor_cheats;}
    LogManager *get_log_manager() {return m_log_mgr;}

    public slots:
        void add_custom_profession();
        void add_custom_profession(CustomProfession *cp);
        void read_settings();
        void write_settings();
        void import_existing_professions();
        void edit_custom_profession();
        void edit_custom_profession(QListWidgetItem*);
        void delete_custom_profession();

private:
    QVector<QString> m_generic_words;
    QVector<QString> m_dwarf_words;
    QVector<Word *> m_language;
    QVector<CustomProfession*> m_custom_professions;
    QSettings *m_user_settings;
    MainWindow *m_main_window;
    OptionsMenu *m_options_menu;
    bool m_reading_settings;
    bool m_allow_labor_cheats;
    LogManager *m_log_mgr;

    void setup_logging();
    void load_translator();


signals:
    void settings_changed();
};

#endif
