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
#include <QtGui>
#include <QtNetwork>
#include <QtDebug>
#include <QxtLogger>

#include "mainwindow.h"
#include "ui_about.h"
#include "ui_mainwindow.h"
#include "ui_pendingchanges.h"
#include "optionsmenu.h"
#include "aboutdialog.h"
#include "dwarfmodel.h"
#include "dwarfmodelproxy.h"
#include "dfinstance.h"
#include "memorylayout.h"
#include "statetableview.h"
#include "viewmanager.h"
#include "viewcolumnset.h"
#include "uberdelegate.h"
#include "customprofession.h"
#include "labor.h"
#include "defines.h"
#include "version.h"
#include "dwarftherapist.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_view_manager(0)
	, m_about_dialog(new AboutDialog(this))
	, m_df(0)
	, m_lbl_status(0)
	, m_settings(0)
	, m_model(new DwarfModel(this))
	, m_proxy(new DwarfModelProxy(this))
	, m_http(new QHttp(this))
	, m_reading_settings(false)
	, m_temp_cp(0)
{
	ui->setupUi(this);
	m_view_manager = new ViewManager(m_model, m_proxy, this);
	ui->v_box->addWidget(m_view_manager);

	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	ui->dock_grid_views->set_view_manager(m_view_manager);
	ui->dock_sets->set_view_manager(m_view_manager);

	ui->menu_docks->addAction(ui->dock_pending_jobs_list->toggleViewAction());
	ui->menu_docks->addAction(ui->dock_custom_professions->toggleViewAction());
	ui->menu_docks->addAction(ui->dock_grid_views->toggleViewAction());
	ui->menu_docks->addAction(ui->dock_sets->toggleViewAction());
	ui->menuWindows->addAction(ui->main_toolbar->toggleViewAction());


	LOGD << "setting up connections for MainWindow";
	connect(m_model, SIGNAL(new_pending_changes(int)), this, SLOT(new_pending_changes(int)));
	connect(ui->act_clear_pending_changes, SIGNAL(triggered()), m_model, SLOT(clear_pending()));
	connect(ui->act_commit_pending_changes, SIGNAL(triggered()), m_model, SLOT(commit_pending()));
	connect(ui->act_expand_all, SIGNAL(triggered()), m_view_manager, SLOT(expand_all()));
	connect(ui->act_collapse_all, SIGNAL(triggered()), m_view_manager, SLOT(collapse_all()));
	connect(ui->dock_grid_views, SIGNAL(views_changed()), m_view_manager, SLOT(views_changed()));
	connect(ui->dock_sets, SIGNAL(sets_changed()), m_view_manager, SLOT(sets_changed()));
	connect(ui->act_add_new_gridview, SIGNAL(triggered()), ui->dock_grid_views, SLOT(add_new_view()));
	connect(ui->act_add_new_column_set, SIGNAL(triggered()), ui->dock_sets, SLOT(add_new_set()));
	connect(ui->list_custom_professions, SIGNAL(customContextMenuRequested(const QPoint &)),
			this, SLOT(draw_custom_profession_context_menu(const QPoint &)));

	connect(ui->tree_pending, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
			m_view_manager, SLOT(jump_to_dwarf(QTreeWidgetItem *, QTreeWidgetItem *)));
	connect(ui->list_custom_professions, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		m_view_manager, SLOT(jump_to_profession(QListWidgetItem *, QListWidgetItem *)));


	m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, COMPANY, PRODUCT, this);

	m_lbl_status = new QLabel(tr("not connected"), statusBar());
	statusBar()->addPermanentWidget(m_lbl_status, 0);
	set_interface_enabled(false);

	ui->cb_group_by->setItemData(0, DwarfModel::GB_NOTHING);
	ui->cb_group_by->addItem("Profession", DwarfModel::GB_PROFESSION);
	ui->cb_group_by->addItem("Legendary or not", DwarfModel::GB_LEGENDARY);
	ui->cb_group_by->addItem("Sex", DwarfModel::GB_SEX);
	ui->cb_group_by->addItem("Happiness", DwarfModel::GB_HAPPINESS);
	
	read_settings();
	draw_professions();

	check_latest_version();
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::read_settings() {
	m_reading_settings = true;
	m_settings->beginGroup("window");
	{ // WINDOW SETTINGS
		QByteArray geom = m_settings->value("geometry").toByteArray();
		if (!geom.isEmpty()) {
			restoreGeometry(geom);
		}
		// Toolbars etc... 
		QByteArray state = m_settings->value("state").toByteArray();
		if (!state.isEmpty()) {
			restoreState(state);
		}
	}
	m_settings->endGroup();

	m_settings->beginGroup("gui_options");
	{ // GUI OPTIONS
		int group_by = m_settings->value("group_by", 0).toInt();
		ui->cb_group_by->setCurrentIndex(group_by);
		m_model->set_group_by(group_by);
	}
	m_settings->endGroup();

	m_reading_settings = false;
}

void MainWindow::write_settings() {
	if (m_settings && !m_reading_settings) {
		LOGD << "beginning to write settings";
		QByteArray geom = saveGeometry();
		QByteArray state = saveState();
		m_settings->beginGroup("window");
		m_settings->setValue("geometry", QVariant(geom));
		m_settings->setValue("state", QVariant(state));
		m_settings->endGroup();
		m_settings->beginGroup("gui_options");
		m_settings->setValue("group_by", m_model->current_grouping());
		m_settings->endGroup();

		LOGD << "finished writing settings";
	}
}

void MainWindow::closeEvent(QCloseEvent *evt) {
	LOG->info() << "Beginning shutdown";
	write_settings();
	m_view_manager->write_views();
	evt->accept();
	LOG->info() << "Closing Dwarf Therapist normally";
}

void MainWindow::connect_to_df() {
	LOGD << "attempting connection to running DF game";
	if (m_df) {
		LOGD << "already connected, disconnecting";
		delete m_df;
		set_interface_enabled(false);
		m_df = 0;
	}
	// find_running_copy can fail for several reasons, and will take care of 
	// logging and notifying the user.
	m_df = DFInstance::find_running_copy(this);
	if (m_df && m_df->is_ok()) {
		m_lbl_status->setText(tr("Connected to ") + m_df->memory_layout()->game_version());
		set_interface_enabled(true);
	}
}

void MainWindow::read_dwarves() {
	if (!m_df || !m_df->is_ok()) {
		return;
	}
	m_model->set_instance(m_df);
	m_model->load_dwarves();
	// cheap trick to setup the view correctly
	m_view_manager->redraw_current_tab();
}

void MainWindow::set_interface_enabled(bool enabled) {
	ui->act_connect_to_DF->setEnabled(!enabled);
	ui->act_read_dwarves->setEnabled(enabled);
	ui->act_scan_memory->setEnabled(enabled);
	ui->act_expand_all->setEnabled(enabled);
	ui->act_collapse_all->setEnabled(enabled);
	ui->cb_group_by->setEnabled(enabled);
	ui->btn_import_professions->setEnabled(enabled);
	ui->act_import_existing_professions->setEnabled(enabled);
}

void MainWindow::check_latest_version() {
	//http://code.google.com/p/dwarftherapist/wiki/LatestVersion
	Version our_v(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	QHttpRequestHeader header("GET", "/version");
	header.setValue("Host", "dt-tracker.appspot.com");
	//header.setValue("Host", "localhost");
	header.setValue("User-Agent", QString("DwarfTherapist %1").arg(our_v.to_string()));
	m_http->setHost("dt-tracker.appspot.com");
	//m_http->setHost("localhost", 8080);
	disconnect(m_http, SIGNAL(done(bool)));
	connect(m_http, SIGNAL(done(bool)), this, SLOT(version_check_finished(bool)));
	m_http->request(header);
}

void MainWindow::version_check_finished(bool error) {
	if (error) {
		qWarning() << m_http->errorString();
	}
	QString data = QString(m_http->readAll());
	QRegExp rx("(\\d+)\\.(\\d+)\\.(\\d+)");
	int pos = rx.indexIn(data);
	if (pos != -1) {
		Version our_v(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
		QString major = rx.cap(1);
		QString minor = rx.cap(2);
		QString patch = rx.cap(3);
		Version newest_v(major.toInt(), minor.toInt(), patch.toInt());
		LOGI << "OUR VERSION:" << our_v.to_string();
		LOGI << "LATEST VERSION:" << newest_v.to_string();
		if (our_v < newest_v) {
			LOGI << "LATEST VERSION IS NEWER!";
			QMessageBox *mb = new QMessageBox(this);
			mb->setWindowTitle(tr("Update Available"));
			mb->setText(tr("A newer version of this application is available."));
			QString link = "<a href=\"http://code.google.com/p/dwarftherapist/downloads/list\">Download v" + newest_v.to_string() + "</a>";
			mb->setInformativeText(QString("You are currently running v%1. %2").arg(our_v.to_string()).arg(link));
			mb->exec();
		}
		m_about_dialog->set_latest_version(newest_v);
	} else {
		m_about_dialog->version_check_failed();
		
	}
}

void MainWindow::scan_memory() {
	QProgressDialog *pd = new QProgressDialog(tr("Scanning Memory"), tr("Cancel"), 0, 1, this);
	connect(m_df, SIGNAL(scan_total_steps(int)), pd, SLOT(setMaximum(int)));
	connect(m_df, SIGNAL(scan_progress(int)), pd, SLOT(setValue(int)));
	connect(m_df, SIGNAL(scan_message(QString)), pd, SLOT(setLabelText(QString)));
	connect(pd, SIGNAL(canceled()), m_df, SLOT(cancel_scan()));
	pd->show();

	int language_addr = m_df->find_language_vector();
	int translation_addr = m_df->find_translation_vector();
	int creature_addr = m_df->find_creature_vector();
	pd->deleteLater();

	qDebug() << "LANGUAGE VECTOR:   " << hex << language_addr;
	qDebug() << "TRANSLATION VECTOR:" << hex << translation_addr;
	qDebug() << "CREATURE VECTOR:   " << hex << creature_addr;
}

void MainWindow::set_group_by(int group_by) {
	write_settings();
	m_view_manager->set_group_by(group_by);
}

void MainWindow::show_about() {
	m_about_dialog->show();
}

void MainWindow::new_pending_changes(int cnt) {
	bool on_off = cnt > 0;
	ui->lbl_pending_changes->setNum(cnt);
	ui->btn_clear->setEnabled(on_off);
	ui->btn_commit->setEnabled(on_off);
	ui->act_clear_pending_changes->setEnabled(on_off);
	ui->act_commit_pending_changes->setEnabled(on_off);
	list_pending();
}

void MainWindow::list_pending() {
	disconnect(ui->tree_pending, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		0, 0);
	ui->tree_pending->clear();
	foreach(Dwarf *d, m_model->get_dirty_dwarves()) {
		ui->tree_pending->addTopLevelItem(d->get_pending_changes_tree());
	}
	ui->tree_pending->expandAll();
	connect(ui->tree_pending, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		m_view_manager, SLOT(jump_to_dwarf(QTreeWidgetItem *, QTreeWidgetItem *)));
}

void MainWindow::draw_professions() {
	disconnect(ui->list_custom_professions, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		0, 0);
	ui->list_custom_professions->clear();
	foreach(CustomProfession *cp, DT->get_custom_professions()) {
		new QListWidgetItem(cp->get_name(), ui->list_custom_professions);
	}
	connect(ui->list_custom_professions, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		m_view_manager, SLOT(jump_to_profession(QListWidgetItem *, QListWidgetItem *)));

}

void MainWindow::draw_custom_profession_context_menu(const QPoint &p) {
	QModelIndex idx = ui->list_custom_professions->indexAt(p);
	if (!idx.isValid())
		return;

	QString cp_name = idx.data().toString();

	QMenu m(this);
	m.setTitle(tr("Custom Profession"));
	QAction *a = m.addAction(tr("Edit..."), DT, SLOT(edit_custom_profession()));
	a->setData(cp_name);
	a = m.addAction(tr("Delete..."), DT, SLOT(delete_custom_profession()));
	a->setData(cp_name);
	m.exec(ui->list_custom_professions->viewport()->mapToGlobal(p));
}

// web addresses
void MainWindow::go_to_forums() {
	QDesktopServices::openUrl(QUrl("http://udpviper.com/forums/viewforum.php?f=36"));
}
void MainWindow::go_to_donate() {
	QDesktopServices::openUrl(QUrl("http://code.google.com/p/dwarftherapist/wiki/Donations"));
}
void MainWindow::go_to_project_home() {
	QDesktopServices::openUrl(QUrl("http://code.google.com/p/dwarftherapist"));
}
void MainWindow::go_to_new_issue() {
	QDesktopServices::openUrl(QUrl("http://code.google.com/p/dwarftherapist/issues/entry"));
}

QToolBar *MainWindow::get_toolbar() {
	return ui->main_toolbar;
}