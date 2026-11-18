#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <functional>

class DashboardView : public QWidget {
    Q_OBJECT

public:
    explicit DashboardView(QWidget* parent = nullptr);

signals:
    void navigateToCashier();
    void navigateToProductDescription();
    void navigateToOrders();
    void navigateToManager();

private:
    void setupUi();
    QFrame* createCard(const QString& title,
                       const QString& description,
                       const QString& buttonText,
                       std::function<void()> onClick);
};
