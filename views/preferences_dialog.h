#pragma once

#include <QDialog>

class QComboBox;
class QLineEdit;
class QPushButton;

namespace NezhaIDE::Views {

class PreferencesDialog final : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

private:
    void setupUi();
    void loadSettings();
    void applySettings();

    QComboBox *theme_combo_{};
    QComboBox *language_combo_{};
    QComboBox *llm_combo_{};
    QLineEdit *api_token_edit_{};
};

} // namespace NezhaIDE::Views
