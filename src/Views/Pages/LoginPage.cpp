/**
 * @file LoginPage.cpp
 * @brief Implements the LoginPage class, handling user login/logout UI and interactions.
 */

#include "TechMa/Views/Pages/LoginPage.h"
#include "TechMa/Authentication/UserAuthentication.h"

#include <QColor>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

/**
 * @brief Constructs the login page UI and connects signals to slots.
 * 
 * @param parent Optional parent QWidget.
 */
LoginPage::LoginPage(QWidget *parent) : QWidget(parent)
{
  setup_ui();
  setup_connects();
}

/**
 * @brief Builds the full UI layout for the login interface.
 */
void LoginPage::setup_ui()
{
    auto *main_layout = new QVBoxLayout(this);
    main_layout->setAlignment(Qt::AlignCenter);
    main_layout->setContentsMargins(40, 40, 40, 40);
    main_layout->setSpacing(20);

    auto *card = new QWidget(this);
    auto *card_layout = new QVBoxLayout(card);
    card_layout->setSpacing(20);
    card_layout->setContentsMargins(30, 30, 30, 30);
    card_layout->setAlignment(Qt::AlignCenter);

    auto *title_label = new QLabel("TechMa Account", card);
    title_label->setObjectName("Title");
    title_label->setAlignment(Qt::AlignCenter);

    auto *form_container = new QWidget(card);
    auto *form_container_layout = new QFormLayout(form_container);
    form_container_layout->setLabelAlignment(Qt::AlignRight);
    form_container_layout->setFormAlignment(Qt::AlignHCenter);
    form_container_layout->setHorizontalSpacing(15);
    form_container_layout->setVerticalSpacing(10);

    m_username_input = new QLineEdit(form_container);
    m_username_input->setPlaceholderText("Username");
    m_username_input->setMaximumWidth(300);

    m_password_input = new QLineEdit(form_container);
    m_password_input->setPlaceholderText("Password");
    m_password_input->setEchoMode(QLineEdit::Password);
    m_password_input->setMaximumWidth(300);

    form_container_layout->addRow("Username:", m_username_input);
    form_container_layout->addRow("Password:", m_password_input);

    auto *btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(15);
    btn_layout->setAlignment(Qt::AlignCenter);

    m_login_button = new QPushButton("Login", card);
    m_login_button->setIcon(tinted_icon(":/styles/icons/navigation_bar/log-in.svg", QColor("#6B6BFF")));
    m_login_button->setMaximumWidth(150);

    m_guest_button = new QPushButton("Continue as Guest", card);
    m_guest_button->setMaximumWidth(150);

    m_logout_button = new QPushButton("Logout", card);
    m_logout_button->setIcon(tinted_icon(":/styles/icons/navigation_bar/log-out.svg", QColor("#FF6B6B")));
    m_logout_button->setVisible(false);
    m_logout_button->setMaximumWidth(150);

    btn_layout->addWidget(m_login_button);
    btn_layout->addWidget(m_guest_button);
    btn_layout->addWidget(m_logout_button);

    m_status_label = new QLabel(card);
    m_status_label->setAlignment(Qt::AlignCenter);

    m_user_info = new QLabel(card);
    m_user_info->setAlignment(Qt::AlignCenter);
    m_user_info->setVisible(false);

    auto *hints_label = new QLabel(card);
    hints_label->setText(
        "Available accounts for demo:\n"
        "- Username: admin, Password: admin123 (Admin)\n"
        "- Username: tech, Password: tech123 (Technician)\n"
        "- Username: guest, Password: guest123 (Guest)"
    );
    hints_label->setAlignment(Qt::AlignCenter);

    card_layout->addWidget(title_label);
    card_layout->addWidget(form_container);        
    card_layout->addSpacing(10);
    card_layout->addLayout(btn_layout);
    card_layout->addWidget(m_status_label);
    card_layout->addWidget(m_user_info);
    card_layout->addSpacing(20);
    card_layout->addWidget(hints_label);

    main_layout->addWidget(card);
    setLayout(main_layout);
}

/**
 * @brief Connects signals to login-related slots and updates button states.
 */
void LoginPage::setup_connects()
{
  connect(m_login_button, &QPushButton::clicked, this, &LoginPage::attempt_login);
  connect(m_guest_button, &QPushButton::clicked, this, &LoginPage::login_as_guest);
  connect(m_logout_button, &QPushButton::clicked, this, &LoginPage::logout);

  connect(m_username_input, &QLineEdit::returnPressed, this, &LoginPage::attempt_login);
  connect(m_password_input, &QLineEdit::returnPressed, this, &LoginPage::attempt_login);

  auto update_login_button = [this]() {
    m_login_button->setEnabled(
        !m_username_input->text().isEmpty() &&
        !m_password_input->text().isEmpty()
    );
  };

  connect(m_username_input, &QLineEdit::textChanged, this, update_login_button);
  connect(m_password_input, &QLineEdit::textChanged, this, update_login_button);

  update_login_button();
}

/**
 * @brief Attempts to log the user in using the entered credentials.
 */
void LoginPage::attempt_login()
{
  QString username = m_username_input->text();
  QString password = m_password_input->text();

  if (AuthenticationService::instance().login(username.toStdString(), hash_password(password.toStdString()))) {
    auto current_user = AuthenticationService::instance().current_user();
    update_ui_for_logged_in_user(current_user);
    emit login_successful(current_user->role());
    clear_fields();
  } else {
    m_status_label->setText("Invalid username or password");
    QTimer::singleShot(3000, m_status_label, &QLabel::clear);
  }
}

/**
 * @brief Logs the user in as a guest.
 */
void LoginPage::login_as_guest()
{
  AuthenticationService::instance().logout();
  emit login_successful(UserRole::GUEST);
  clear_fields();
}

/**
 * @brief Logs the user out and resets the UI.
 */
void LoginPage::logout()
{
  AuthenticationService::instance().logout();
  update_ui_for_logged_out();
  emit logout_requested();
}

/**
 * @brief Updates the UI to show the logged-in state.
 * 
 * @param user The currently logged-in user.
 */
void LoginPage::update_ui_for_logged_in_user(std::shared_ptr<User> user)
{
  if (!user)
    return;

  m_username_input->setVisible(false);
  m_password_input->setVisible(false);
  m_login_button->setVisible(false);
  m_guest_button->setVisible(false);

  QString role_name;
  switch (user->role()) {
    case UserRole::ADMIN:      role_name = "Admin"; break;
    case UserRole::TECHNICIAN: role_name = "Technician"; break;
    case UserRole::GUEST:      role_name = "Guest"; break;
  }

  m_user_info->setText(QString("Logged in as: %1 (%2)")
                           .arg(QString::fromStdString(user->username()))
                           .arg(role_name));
  m_user_info->setVisible(true);
  m_logout_button->setVisible(true);
}

/**
 * @brief Updates the UI to show the logged-out state.
 */
void LoginPage::update_ui_for_logged_out()
{
  m_username_input->setVisible(true);
  m_password_input->setVisible(true);
  m_login_button->setVisible(true);
  m_guest_button->setVisible(true);

  m_user_info->setVisible(false);
  m_logout_button->setVisible(false);
}

/**
 * @brief Clears all login form fields and status messages.
 */
void LoginPage::clear_fields()
{
  m_username_input->clear();
  m_password_input->clear();
  m_status_label->clear();
}

/**
 * @brief Qt show event override that updates the UI state based on login status.
 * 
 * @param event The show event.
 */
void LoginPage::showEvent(QShowEvent *event)
{
  QWidget::showEvent(event);

  auto current_user = AuthenticationService::instance().current_user();
  if (current_user) {
    update_ui_for_logged_in_user(current_user);
  } else {
    update_ui_for_logged_out();
  }
}

/**
 * @brief Returns a tinted version of an icon loaded from a given file path.
 * 
 * @param path File path to the icon.
 * @param color Tint color.
 * @return A QIcon with the applied tint.
 */
QIcon LoginPage::tinted_icon(const QString &path, const QColor &color) const
{
  QPixmap src(path);
  if (src.isNull()) {
    return QIcon();
  }

  QPixmap dst(src.size());
  dst.fill(Qt::transparent);

  QPainter p(&dst);
  if (!p.isActive()) {
    return QIcon(src);
  }

  p.setCompositionMode(QPainter::CompositionMode_Source);
  p.drawPixmap(0, 0, src);
  p.setCompositionMode(QPainter::CompositionMode_SourceIn);
  p.fillRect(dst.rect(), color);
  p.end();

  return QIcon(dst);
}