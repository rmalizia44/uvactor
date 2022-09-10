#ifndef SQLITE_REACTOR_HPP
#define SQLITE_REACTOR_HPP

#include <sqlite3.h>
#include "reactor.hpp"
#include "common-events.hpp"
#include "database-events.hpp"

class SqliteException: public std::exception {
public:
	explicit SqliteException(int ec): _str(sqlite3_errstr(ec)) {}
	explicit SqliteException(sqlite3* db): _str(sqlite3_errmsg(db)) {}
	const char* what() const noexcept override {
		return _str.c_str();
	}
private:
	std::string _str;
};

#define SQLITE_INVOKE(_db_expr_, _ec_expr_) {\
	int _ec_ = (_ec_expr_);\
	sqlite3* _db_ = (_db_expr_);\
	if(_ec_ != SQLITE_OK) {\
		if(_db_ != nullptr) {\
			throw SqliteException(_db_);\
		} else {\
			throw SqliteException(_ec_);\
		}\
	}\
}

class SqliteDatabase {
public:
	explicit SqliteDatabase(const char* filename) {
		SQLITE_INVOKE(_db, sqlite3_open(filename, &_db));
	}
	explicit SqliteDatabase(const char* filename, const char* sql): SqliteDatabase(filename) {
		exec(sql);
	}
	~SqliteDatabase() {
		if(_db != nullptr) {
			/*SQLITE_INVOKE*/(sqlite3_close_v2(_db));
		}
	}
	void exec(const char* sql) {
		SQLITE_INVOKE(_db, sqlite3_exec(_db, sql, nullptr, nullptr, nullptr)); // TODO: test
	}
	sqlite3* raw() {
		return _db;
	}
private:
	sqlite3* _db;
};

class SqliteQuery {
	struct StmtReseter { void operator()(sqlite3_stmt* stmt) const noexcept { sqlite3_reset(stmt); } };
	using StmtHolder = std::unique_ptr<sqlite3_stmt, StmtReseter>;
public:
	explicit SqliteQuery(sqlite3* db, sqlite3_stmt* stmt) noexcept: _db(db), _stmt(stmt) {
	}
	template<typename... Ts>
	bool row(Ts&&... ts) {
		bool has_value = step();
		if(has_value) {
			column<Ts...>(std::forward<Ts>(ts)...);
		}
		return has_value;
	}
private:
	template<typename... Ts>
	void column(Ts&&... ts) {
		template_column<0, Ts...>(std::forward<Ts>(ts)...);
	}
	bool step() {
		int ec = sqlite3_step(_stmt.get());
		//printf("row result = %i\n", ec);
		if(ec == SQLITE_ROW) {
			return true;
		} else if(ec != SQLITE_DONE) {
			SQLITE_INVOKE(_db, ec);
		}
		return false;
	}
	template<unsigned I>
	void template_column() {
	}
	template<unsigned I, typename T, typename... Ts>
	void template_column(T&& t, Ts&&... ts) {
		do_column(I, std::forward<T>(t));
		template_column<I+1, Ts...>(std::forward<Ts>(ts)...);
	}
	void do_column(int pos, int& value) {
		value = sqlite3_column_int(_stmt.get(), pos);
	}
	void do_column(int pos, double& value) {
		value = sqlite3_column_double(_stmt.get(), pos);
	}
	void do_column(int pos, const char*& value) {
		value = reinterpret_cast<const char*>(sqlite3_column_text(_stmt.get(), pos));
	}
	void do_column(int pos, std::string& value) {
		value.assign(reinterpret_cast<const char*>(sqlite3_column_blob(_stmt.get(), pos)), sqlite3_column_bytes(_stmt.get(), pos));
	}
	sqlite3* _db;
	StmtHolder _stmt;
};

class SqliteStmt {
public:
	SqliteStmt(SqliteDatabase& database, const char* sql): _db(database.raw()) {
		SQLITE_INVOKE(_db, sqlite3_prepare_v2(_db, sql, -1, &_stmt, nullptr));
	}
	~SqliteStmt() {
		if(_stmt != nullptr) {
			/*SQLITE_INVOKE*/(sqlite3_finalize(_stmt));
		}
	}
	template<typename... Ts>
	SqliteQuery bind(Ts&&... ts) {
		template_bind<1, Ts...>(std::forward<Ts>(ts)...);
		return SqliteQuery(_db, _stmt);
	}
private:
	template<unsigned I>
	void template_bind() {
	}
	template<unsigned I, typename T, typename... Ts>
	void template_bind(T&& t, Ts&&... ts) {
		do_bind(I, std::forward<T>(t));
		template_bind<I+1, Ts...>(std::forward<Ts>(ts)...);
	}
	void do_bind(int pos, int value) {
		SQLITE_INVOKE(_db, sqlite3_bind_int(_stmt, pos, value));
	}
	void do_bind(int pos, double value) {
		SQLITE_INVOKE(_db, sqlite3_bind_double(_stmt, pos, value));
	}
	void do_bind(int pos, const char* value) {
		SQLITE_INVOKE(_db, sqlite3_bind_text(_stmt, pos, value, -1, SQLITE_TRANSIENT));
	}
	void do_bind(int pos, const std::string& value) {
		SQLITE_INVOKE(_db, sqlite3_bind_blob(_stmt, pos, value.c_str(), value.size(), SQLITE_TRANSIENT));
	}
	sqlite3* _db;
	sqlite3_stmt* _stmt;
};

class SqliteReactor: public Reactor {
public:
	explicit SqliteReactor(SelfPtr self, ActorPtr observer, ActorPtr logger): _self(self), _observer(observer), _logger(logger), _db(":memory:", "create table test(key varchar(256), value varchar(256));"), _stmt_put(_db, "insert into test (key, value) values (?, ?);"), _stmt_get(_db, "select value from test where key=?;") {
	}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtPut::TYPE:
				on_put(event->as<EvtPut>(), timestamp);
				break;
			case EvtGet::TYPE:
				on_get(event->as<EvtGet>(), timestamp);
				break;
			case EvtExit::TYPE:
				_self->reset();
				break;
			default:
				break;
		}
	}
private:
	void on_put(const EvtPut& event, uint32_t timestamp) {
		_logger->send(
			Event::make<EvtLog>("[D] put %s: %s", event.key.c_str(), event.value.c_str())
		);
		_stmt_put.bind(event.key, event.value).row();
	}
	void on_get(const EvtGet& event, uint32_t timestamp) {
		_logger->send(
			Event::make<EvtLog>("[D] get %s", event.key.c_str())
		);
		const char* value;
		std::string result;
		auto query = _stmt_get.bind(event.key);
		if(query.row(value)) {
			result = value;
		}
		_logger->send(
			Event::make<EvtLog>("\tresult: %s", result.c_str())
		);
		_observer->send(
			Event::make<EvtResult>(
				std::string(event.key),
				std::move(result)
			)
		);
	}
	
	SelfPtr _self;
	ActorPtr _observer;
	ActorPtr _logger;
	SqliteDatabase _db;
	SqliteStmt _stmt_put;
	SqliteStmt _stmt_get;
};

#endif