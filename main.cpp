#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "ApiHandler.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    try {
        //ustawienie stylu aplikacji (opcjonalne)
        QApplication::setStyle("Fusion");

        //inicjalizacja głównego okna
        MainWindow mainWindow;
        mainWindow.show();

        return app.exec();

    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Błąd krytyczny",
                              QString("Wystąpił nieoczekiwany błąd:\n%1\nAplikacja zostanie zamknięta.").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Błąd krytyczny",
                              "Wystąpił nieznany błąd.\nAplikacja zostanie zamknięta.");
        return -1;
    }
}
