#include "stdafx.h"
#include "SearchModel.h"
#include "ContactListModel.h"
#include "ContactItem.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"


namespace Logic
{
	SearchModel::SearchModel(QObject *parent)
		: AbstractSearchModel(parent)
		, SearchRequested_(false)
	{
		connect(Ui::GetDispatcher(), SIGNAL(searchResult(QStringList)), this, SLOT(searchResult(QStringList)), Qt::QueuedConnection);
	}

	int SearchModel::rowCount(const QModelIndex &) const
	{
		return (int)Match_.size();
	}
	
	QVariant SearchModel::data(const QModelIndex & ind, int role) const
	{
		if (!ind.isValid() || (role != Qt::DisplayRole && !Testing::isAccessibleRole(role)) || (unsigned)ind.row() >= Match_.size())
			return QVariant();

        if (Testing::isAccessibleRole(role))
            return Match_[ind.row()].get_aimid();

		return QVariant::fromValue(Match_[ind.row()].Get());
	}

	Qt::ItemFlags SearchModel::flags(const QModelIndex &) const
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}

	void SearchModel::setFocus()
	{
		Match_.clear();
	}

	const QStringList& SearchModel::GetPattern() const
	{
		return SearchPatterns_;
	}

	void SearchModel::searchPatternChanged(QString p)
	{
		SearchPatterns_ = Utils::GetPossibleStrings(p);
		if (p.isEmpty())
		{
			unsigned size = (unsigned)Match_.size();
			Match_ = GetContactListModel()->GetSearchedContacts();
			emit dataChanged(index(0), index(size));
			return;
		}

		if (!SearchRequested_)
		{
			QTimer::singleShot(200, [this]()
			{
				SearchRequested_ = false;
				Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
				core::ifptr<core::iarray> patternsArray(collection->create_array());
				patternsArray->reserve(SearchPatterns_.size());
				for (auto iter = SearchPatterns_.begin(); iter != SearchPatterns_.end(); ++iter)
				{
					core::coll_helper coll(collection->create_collection(), true);
					coll.set_value_as_string("pattern", iter->toUtf8().data(), iter->toUtf8().size());
					core::ifptr<core::ivalue> val(collection->create_value());
					val->set_as_collection(coll.get());
					patternsArray->push_back(val.get());
				}
				collection.set_value_as_array("search_patterns", patternsArray.get());
				Ui::GetDispatcher()->post_message_to_core("search", collection.get());
			});
			SearchRequested_ = true;
		}
	}

	void SearchModel::searchResult(QStringList result)
	{
		unsigned size = (unsigned)Match_.size();
		Match_ = GetContactListModel()->GetSearchedContacts(result.toStdList());
		emit dataChanged(index(0), index(size));
		if (!result.isEmpty())
			emit results();
	}

	void SearchModel::emitChanged(int first, int last)
	{
		emit dataChanged(index(first), index(last));
	}

	SearchModel* GetSearchModel()
	{	
		static std::unique_ptr<SearchModel> model(new SearchModel(0));
		return model.get();
	}
}