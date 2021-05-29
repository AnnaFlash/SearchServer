#include "Request.h"
RequestQueue::RequestQueue(const SearchServer& search_server)
{
	search_server_ = &search_server;
}
template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) 
{
	vector<Document> result = (*search_server_).FindTopDocuments(raw_query, document_predicate);
	UpdateRequests(result);
	return result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status)
{

	vector<Document> result = (*search_server_).FindTopDocuments(raw_query, status);
	UpdateRequests(result);
	return vector<Document>(result);
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query)
{
	vector<Document> result = (*search_server_).FindTopDocuments(raw_query);
	UpdateRequests(result);
	return vector<Document>(result);
}

void RequestQueue::UpdateRequests(vector<Document> result)
{
	if (result.empty()) {
		if (requests_.empty()) {
			requests_.push_back(1);
		}
		else if (requests_.front().timestamp == sec_in_day_) {
			requests_.pop_front();
			for (auto& req : requests_) {
				req.timestamp++;
			}
			requests_.push_back(1);
		}
		else {
			for (auto& req : requests_) {
				req.timestamp++;
			}
			requests_.push_back(1);
		}
	}
	else {
		if (!requests_.empty()) {
			if (requests_.front().timestamp == sec_in_day_) {
				requests_.pop_front();
				for (auto& req : requests_) {
					req.timestamp++;
				}
			}
			else {
				for (auto& req : requests_) {
					req.timestamp++;
				}
			}
		}
	}
}

