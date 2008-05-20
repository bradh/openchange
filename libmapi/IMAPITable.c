/*
   OpenChange MAPI implementation.

   Copyright (C) Julien Kerihuel 2007-2008.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libmapi/libmapi.h>
#include <libmapi/proto_private.h>
#include <gen_ndr/ndr_exchange.h>

/**
   \file IMAPITable.c

   \brief Operations on tables
 */


/**
   \details Defines the particular properties and order of properties
  to appear as columns in the table.
 
  \param obj_table the table the function is setting columns for
  \param properties the properties intended to be set

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_W_ERROR_RETURNED: Problem encountered while trying to set
   one or more properties
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

  \sa QueryRows, QueryColumns, SeekRow, GetLastError
 */
_PUBLIC_ enum MAPISTATUS SetColumns(mapi_object_t *obj_table, 
				    struct SPropTagArray *properties)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	TALLOC_CTX		*mem_ctx;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct SetColumns_req	request;
	uint32_t		size;
	mapi_object_table_t	*table;
	mapi_ctx_t		*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("SetColumns");

	size = 0;

	/* Fill the SetColumns operation */
	request.unknown = 0;
	request.prop_count = properties->cValues;
	request.properties = properties->aulPropTag;
	size += 3 + request.prop_count * sizeof (uint32_t);

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_SetColumns;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_SetColumns = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval && (retval != MAPI_W_ERRORS_RETURNED), retval, mem_ctx);

	/* recopy property tags into table */
	/* fixme: obj_table->private_data should be initialized during opening, not here */
	if (obj_table->private_data == 0) {
		obj_table->private_data = talloc((TALLOC_CTX *)mapi_ctx->session, mapi_object_table_t);
	}

	table = (mapi_object_table_t *)obj_table->private_data;
	if (table) {
		table->proptags.cValues = properties->cValues;
		table->proptags.aulPropTag = talloc_array((TALLOC_CTX *)obj_table->private_data,
							  enum MAPITAGS, table->proptags.cValues);
		memcpy((void*)table->proptags.aulPropTag, (void*)properties->aulPropTag,
		       table->proptags.cValues * sizeof(enum MAPITAGS));
	}

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details returns the total number of rows in the table
 
   \param obj_table pointer to the table's object
   \param cn_rows pointer to the total number of rows in the table

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa QueryRows
*/
_PUBLIC_ enum MAPISTATUS GetRowCount(mapi_object_t *obj_table, 
				     uint32_t *cn_rows)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("GetRowCount");

	*cn_rows = 0;
	size = 0;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_GetRowCount;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);
	
	*cn_rows = mapi_response->mapi_repl->u.mapi_GetRowCount.count;
	
	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Returns a RowSet with the properties returned by the
   server

   \param obj_table the table we are requesting properties from
   \param row_count the maximum number of rows to retrieve
   \param flags flags to use for the query
   \param rowSet the results

   flags possible values:
   - TBL_ADVANCE: index automatically increased from last rowcount
   - TBL_NOADVANCE: should be used for a single QueryRows call
   - TBL_ENABLEPACKEDBUFFERS: (not yet implemented)

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa SetColumns, GetRowCount, QueryColumns, SeekRow
 */
_PUBLIC_ enum MAPISTATUS QueryRows(mapi_object_t *obj_table, uint16_t row_count,
				   enum QueryRowsFlags flags, 
				   struct SRowSet *rowSet)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct QueryRows_req	request;
	struct QueryRows_repl	*reply;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size = 0;
	TALLOC_CTX		*mem_ctx;
	mapi_object_table_t	*table;
	mapi_ctx_t		*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("QueryRows");

	/* Fill the QueryRows operation */
	request.QueryRowsFlags = flags;
	/* TODO: search backwards for negative row_count */
	request.ForwardRead = 1;
	request.RowCount = row_count;
	size += 4;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_QueryRows;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_QueryRows = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	/* table contains mapitags from previous SetColumns */
	table = (mapi_object_table_t *)obj_table->private_data;
	MAPI_RETVAL_IF(!table, MAPI_E_INVALID_OBJECT, mem_ctx);

	/* TODO: handle Origin */
	reply = &mapi_response->mapi_repl->u.mapi_QueryRows;
	rowSet->cRows = reply->RowCount;
	rowSet->aRow = talloc_array((TALLOC_CTX *)table, struct SRow, rowSet->cRows);
	emsmdb_get_SRowSet((TALLOC_CTX *)table, rowSet, &table->proptags, &reply->RowData);

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Retrieves the set of columns defined in the current table
   view

   \param obj_table the table we are retrieving columns from
   \param cols pointer to an array of property tags

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
     transaction

   \sa SetColumns, QueryRows
*/
_PUBLIC_ enum MAPISTATUS QueryColumns(mapi_object_t *obj_table, 
				      struct SPropTagArray *cols)
{
	struct mapi_request		*mapi_request;
	struct mapi_response		*mapi_response;
	struct EcDoRpc_MAPI_REQ		*mapi_req;
	struct QueryColumnsAll_repl	*reply;
	NTSTATUS			status;
	enum MAPISTATUS			retval;
	uint32_t			size;
	TALLOC_CTX			*mem_ctx;
	mapi_object_table_t		*table;
	mapi_ctx_t			*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("QueryColumns");

	cols->cValues = 0;
	size = 0;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_QueryColumnsAll;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	/* get columns SPropTagArray */
	table = (mapi_object_table_t *)obj_table->private_data;
	MAPI_RETVAL_IF(!table, MAPI_E_INVALID_OBJECT, mem_ctx);

	reply = &mapi_response->mapi_repl->u.mapi_QueryColumnsAll;
	cols->cValues = reply->PropertyTagCount;
	cols->aulPropTag = talloc_array((TALLOC_CTX*)table, enum MAPITAGS, cols->cValues);
	memcpy((void*)cols->aulPropTag, (const void*)reply->PropertyTags, cols->cValues * sizeof(enum MAPITAGS));

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Move the table cursor at a specific location

   \param obj_table the table we are moving cursor on
   \param origin the table position where we start to seek 
   \param offset a particular offset in the table
   \param row the position of the seeked row is returned in rows

   origin possible values:
   - BOOKMARK_BEGINNING: Beginning of the table
   - BOOKMARK_CURRENT: Current position in the table
   - BOKMARK_END: End of the table

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa SetColumns, QueryRows
*/
_PUBLIC_ enum MAPISTATUS SeekRow(mapi_object_t *obj_table, 
				 enum BOOKMARK origin, 
				 uint32_t offset, uint32_t *row)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct SeekRow_req	request;
	struct SeekRow_repl	*reply;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("SeekRow");

	*row = 0;

	/* Fill the SeekRow operation */
	size = 0;
	request.origin = origin;
	size += 1;
	request.offset = offset;
	size += 4;
	request.unknown_1 = 0;
	size += 1;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_SeekRow;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_SeekRow = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	reply = &mapi_response->mapi_repl->u.mapi_SeekRow;
	*row = reply->row;

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Move the table cursor at a specific location given a
   bookmark

   \param obj_table the table we are moving cursor on
   \param lpbkPosition the bookmarked position 
   \param offset an relative offset to the bookmark
   \param row the position of the seeked row is returned in rows

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_INVALID_BOOKMARK: The bookmark specified is invalid or
     beyond the last row requested
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa CreateBookmark, FreeBookmark
 */
_PUBLIC_ enum MAPISTATUS SeekRowBookmark(mapi_object_t *obj_table,
					 uint32_t lpbkPosition,
					 uint32_t offset,
					 uint32_t *row)
{
	struct mapi_request		*mapi_request;
	struct mapi_response		*mapi_response;
	struct EcDoRpc_MAPI_REQ		*mapi_req;
	struct SeekRowBookmark_req	request;
	struct SeekRowBookmark_repl	*reply;
	NTSTATUS			status;
	enum MAPISTATUS			retval;
	uint32_t			size;
	TALLOC_CTX			*mem_ctx;
	mapi_ctx_t			*mapi_ctx;
	struct SBinary_short   		bin;

	/* Sanity checks */
	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);

	retval = mapi_object_bookmark_find(obj_table, lpbkPosition, &bin);
	MAPI_RETVAL_IF(retval, MAPI_E_INVALID_BOOKMARK, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("SeekRowBookmark");

	/* Fill the SeekRowBookmark operation */
	size = 0;
	request.bookmark.cb = bin.cb;
	size += sizeof (uint16_t);
	request.bookmark.lpb = bin.lpb;
	size += bin.cb;
	request.offset = offset;
	size += sizeof (uint32_t);
	request.unknown = 0x1;
	size += sizeof (uint8_t);

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_SeekRowBookmark;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_SeekRowBookmark = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	reply = &mapi_response->mapi_repl->u.mapi_SeekRowBookmark;
	*row = reply->row;

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Moves the cursor to an approximate fractional position in
   the table

   \param obj_table the table we are moving cursor on
   \param ulNumerator numerator of the fraction representing the table
   position.
   \param ulDenominator denominator of the fraction representing the
   table position

   - If ulDenominator is NULL, then SeekRowApprox returns
   MAPI_E_INVALID_PARAMETER.
   - If ulNumerator is NULL, then SeekRowApprox moves the cursor to
     the beginning of the table. In such situation, SeekRowApprox call
     is similar to SeekRow with BOOKMARK_BEGINNING
   - If ulNumerator and ulDenominator have the same value, then
     SeekRowApprox moves the cursor to the end of the table. In such
     situation, SeekRowApprox call is similar to SeekRow with
     BOOKMARK_END
     
   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_INVALID_BOOKMARK: The bookmark specified is invalid or
   beyond the last row requested
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa SeekRow, SeekRowBookmark
 */
_PUBLIC_ enum MAPISTATUS SeekRowApprox(mapi_object_t *obj_table,
				       uint32_t ulNumerator,
				       uint32_t ulDenominator)
{
	struct mapi_request		*mapi_request;
	struct mapi_response		*mapi_response;
	struct EcDoRpc_MAPI_REQ		*mapi_req;
	struct SeekRowApprox_req	request;
	NTSTATUS			status;
	enum MAPISTATUS			retval;
	uint32_t			size;
	TALLOC_CTX			*mem_ctx;
	mapi_ctx_t			*mapi_ctx;

	/* Sanity checks */
	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);
	MAPI_RETVAL_IF(!ulDenominator, MAPI_E_INVALID_PARAMETER, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("SeekRowApprox");
	
	/* Fill the SeekRowApprox operation */
	size = 0;
	request.ulNumerator = ulNumerator;
	size += sizeof (uint32_t);
	request.ulDenominator = ulDenominator;
	size += sizeof (uint32_t);

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_SeekRowApprox;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_SeekRowApprox = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Marks the table current position

   \param obj_table the table we are creating a bookmark in
   \param lpbkPosition pointer on the bookmark value. This bookmark
   can be passed in a call to the SeekRowBookmark method

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction
   
   \sa SeekRowBookmark, FreeBookmark
 */
_PUBLIC_ enum MAPISTATUS CreateBookmark(mapi_object_t *obj_table, 
					uint32_t *lpbkPosition)
{
	struct mapi_request		*mapi_request;
	struct mapi_response		*mapi_response;
	struct EcDoRpc_MAPI_REQ		*mapi_req;
	struct CreateBookmark_repl	*reply;
	NTSTATUS			status;
	enum MAPISTATUS			retval;
	uint32_t			size;
	TALLOC_CTX			*mem_ctx;
	mapi_ctx_t			*mapi_ctx;
	mapi_object_table_t	       	*mapi_table;
	mapi_object_bookmark_t		*bookmark;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("CreateBookmark");
	
	size = 0;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_CreateBookmark;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	reply = &mapi_response->mapi_repl->u.mapi_CreateBookmark;

	mapi_table = (mapi_object_table_t *)obj_table->private_data;
	MAPI_RETVAL_IF(!mapi_table, MAPI_E_INVALID_PARAMETER, mem_ctx);

	/* Store CreateBookmark data in mapi_object_table private_data */
	bookmark = talloc_zero((TALLOC_CTX *)mapi_table->bookmark, mapi_object_bookmark_t);
	mapi_table->bk_last++;
	bookmark->index = mapi_table->bk_last;
	bookmark->bin.cb = reply->bookmark.cb;
	bookmark->bin.lpb = talloc_array((TALLOC_CTX *)bookmark, uint8_t, reply->bookmark.cb);
	memcpy(bookmark->bin.lpb, reply->bookmark.lpb, reply->bookmark.cb);

	DLIST_ADD(mapi_table->bookmark, bookmark);

	*lpbkPosition = mapi_table->bk_last;

	obj_table->private_data = mapi_table;

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Order the rows of the table based on a criteria

   \param obj_table the table we are ordering rows on
   \param lpSortCriteria pointer on sort criterias to apply

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_INVALID_PARAMETER: lpSortCriteria is NULL
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction
 */
_PUBLIC_ enum MAPISTATUS SortTable(mapi_object_t *obj_table, 
				   struct SSortOrderSet *lpSortCriteria)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct SortTable_req	request;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);
	MAPI_RETVAL_IF(!lpSortCriteria, MAPI_E_INVALID_PARAMETER, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("SortTable");

	/* Fill the SortTable operation */
	size = 0;
	request.unknown = 0;
	size += sizeof (uint8_t);
	request.lpSortCriteria.cSorts = lpSortCriteria->cSorts;
	size += sizeof (uint16_t);
	request.lpSortCriteria.cCategories = lpSortCriteria->cCategories;
	size += sizeof (uint16_t);
	request.lpSortCriteria.cExpanded = lpSortCriteria->cExpanded;
	size += sizeof (uint16_t);
	request.lpSortCriteria.aSort = lpSortCriteria->aSort;
	size += lpSortCriteria->cSorts * (sizeof (uint32_t) + sizeof (uint8_t));

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_SortTable;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_SortTable = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/*
 * Restrict a table 
 */
uint32_t get_mapi_SRestriction_size(struct mapi_SRestriction *res)
{
	uint32_t	size;
	uint32_t	i;

	size = 0;

	size += sizeof (res->rt);

	switch(res->rt) {
	case RES_AND:
		size += sizeof (res->res.resAnd.cRes);
		for (i = 0; i < res->res.resAnd.cRes; i++) {
			size += get_mapi_SRestriction_size((struct mapi_SRestriction *)&(res->res.resAnd.res[i]));
		}
		break;
	case RES_OR:
		size += sizeof (res->res.resOr.cRes);
		for (i = 0; i < res->res.resOr.cRes; i++) {
			size += get_mapi_SRestriction_size((struct mapi_SRestriction *)&(res->res.resOr.res[i]));
		}
		break;
	case RES_CONTENT:
		size += sizeof (res->res.resContent.fuzzy);
		size += sizeof (res->res.resContent.ulPropTag);
		size += sizeof (res->res.resContent.lpProp.ulPropTag);
		size += get_mapi_property_size(&(res->res.resContent.lpProp));
		break;
	case RES_PROPERTY:
		size += sizeof (res->res.resProperty.relop);
		size += sizeof (res->res.resProperty.ulPropTag);
		size += sizeof (res->res.resProperty.lpProp.ulPropTag);
		size += get_mapi_property_size(&(res->res.resProperty.lpProp));
		break;
	case RES_COMPAREPROPS:
		size += sizeof (uint8_t);
		size += sizeof (res->res.resCompareProps.ulPropTag1);
		size += sizeof (res->res.resCompareProps.ulPropTag2);
		break;
	case RES_BITMASK:
		size += sizeof (uint8_t);
		size += sizeof (res->res.resBitmask.ulPropTag);
		size += sizeof (res->res.resBitmask.ulMask);
		break;
	case RES_SIZE:
		size += sizeof (uint8_t);
		size += sizeof (res->res.resSize.ulPropTag);
		size += sizeof (res->res.resSize.size);
		break;
	case RES_EXIST:
		size += sizeof (res->res.resExist.ulPropTag);
		break;
	}
	return (size);
}


/**
   \details Applies a filter to a table, reducing the row set to only
   those rows matching the specified criteria.

   \param obj the object we are filtering
   \param res the filters we want to apply

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa QueryRows
*/
_PUBLIC_ enum MAPISTATUS Restrict(mapi_object_t *obj, struct mapi_SRestriction *res)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct Restrict_req	request;
	struct Restrict_repl	*reply;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;

	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!res, MAPI_E_INVALID_PARAMETER, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("Restrict");

	/* Fill the Restrict operation */
	size = 0;
	request.handle_idx = 0;
	size += sizeof (request.handle_idx);
	request.restrictions = *res;
	size += get_mapi_SRestriction_size(res);

	/* add subcontext size */
	size += sizeof (uint16_t);

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_Restrict;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_Restrict = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	reply = &mapi_response->mapi_repl->u.mapi_Restrict;

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Find the next row in a table that matches specific search
   criteria

   \param obj_table the table we are searching in
   \param res pointer on search criterias
   \param bkOrigin bookmark identifying the row where FindRow should
   begin
   \param ulFlags controls the direction of the search
   \param SRowSet the resulting row

   bkOrigin can either take the value of a bookmark created with
   CreateBookmark or any of the default values:
   - BOOKMARK_BEGINNING
   - BOOKMARK_CURRENT
   - BOOKMARK_END

   ulFlags can be set either to DIR_FORWARD (0x0) or DIR_BACKWARD
   (0x1).

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_INVALID_BOOKMARK: the bookmark specified is invalid or
     beyond the last row requested.
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa CreateBookmark
 */
_PUBLIC_ enum MAPISTATUS FindRow(mapi_object_t *obj_table, 
				 struct mapi_SRestriction *res,
				 uint32_t bkOrigin, uint8_t ulFlags,
				 struct SRowSet *SRowSet)
{
	struct mapi_request    	*mapi_request;
	struct mapi_response   	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct FindRow_req     	request;
	struct FindRow_repl    	*reply;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;
	mapi_object_table_t	*table;
	struct SBinary_short	bin;

	/* Sanity checks */
	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);
	MAPI_RETVAL_IF(!res, MAPI_E_INVALID_PARAMETER, NULL);

	if (bkOrigin >= 3) {
		retval = mapi_object_bookmark_find(obj_table, bkOrigin, &bin);
		MAPI_RETVAL_IF(retval, MAPI_E_INVALID_BOOKMARK, NULL);
	}

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("FindRow");

	/* Fill the FindRow operation */
	size = 0;
	request.ulFlags = ulFlags;
	size += sizeof (uint8_t);
	request.res = *res;
	size += get_mapi_SRestriction_size(res);
	request.origin = (bkOrigin > 3) ? 3 : bkOrigin;
	size += sizeof (uint8_t);
	if (bkOrigin >= 3) {
		request.bookmark.cb = bin.cb;
		request.bookmark.lpb = bin.lpb;
		size += sizeof (uint16_t)+ bin.cb;
	} else {
		request.bookmark.cb = 0;
		request.bookmark.lpb = NULL;
		size += sizeof (uint16_t);
	}

	/* add subcontext size */
	size += sizeof (uint16_t);

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_FindRow;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	mapi_req->u.mapi_FindRow = request;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	/* table contains SPropTagArray from previous SetColumns call */
	table = (mapi_object_table_t *)obj_table->private_data;
	MAPI_RETVAL_IF(!table, MAPI_E_INVALID_OBJECT, mem_ctx);

	reply = &mapi_response->mapi_repl->u.mapi_FindRow;
	SRowSet->cRows = 1;
	SRowSet->aRow = talloc_array((TALLOC_CTX *)table, struct SRow, SRowSet->cRows);
	emsmdb_get_SRowSet((TALLOC_CTX *)table, SRowSet, &table->proptags, &reply->row);

	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Get the status of a table

   \param obj_table the table we are retrieving the status from
   \param TableStatus the table status result

   TableStatus can either hold:
	- TBLSTAT_COMPLETE (0x0)
	- TBLSTAT_SORTING (0x9)
	- TBLSTAT_SORT_ERROR (0xA)
	- TBLSTAT_SETTING_COLS (0xB)
	- TBLSTAT_SETCOL_ERROR (0xD)
	- TBLSTAT_RESTRICTING (0xE)
	- TBLSTAT_RESTRICT_ERROR (0xF)

   \return MAPI_E_SUCCESS on success, otherwise -1.

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_INVALID_BOOKMARK: the bookmark specified is invalid or
     beyond the last row requested.
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction

   \sa SetColumns, Restrict, FindRow, GetHiearchyTable, GetContentsTable
 */
_PUBLIC_ enum MAPISTATUS GetStatus(mapi_object_t *obj_table, uint8_t *TableStatus)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct GetStatus_repl	*reply;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;

	/* Sanity checks */
	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);
	
	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("GetStatus");
	size = 0;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_GetStatus;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	size += 5;

	/* Fill the mapi_request structure */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);
	
	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	/* Retrieve TableStatus */
	reply = &mapi_response->mapi_repl->u.mapi_GetStatus;
	*TableStatus = reply->TableStatus;


	talloc_free(mapi_response);
	talloc_free(mem_ctx);

	return MAPI_E_SUCCESS;
}


/**
   \details Aborts an asynchronous table operation in progress

   \param obj_table the table object where we want to abort an
   asynchronous operation
   \param TableStatus pointer on the table status returned by the
   operation

   \return MAPI_E_SUCCESS on success, otherwise -1.

   \note Developers should call GetLastError() to retrieve the last
   MAPI error code. Possible MAPI error codes are:
   - MAPI_E_NOT_INITIALIZED: MAPI subsystem has not been initialized
   - MAPI_E_INVALID_BOOKMARK: the bookmark specified is invalid or
     beyond the last row requested.
   - MAPI_E_CALL_FAILED: A network problem was encountered during the
   transaction
 */
_PUBLIC_ enum MAPISTATUS Abort(mapi_object_t *obj_table, uint8_t *TableStatus)
{
	struct mapi_request	*mapi_request;
	struct mapi_response	*mapi_response;
	struct EcDoRpc_MAPI_REQ	*mapi_req;
	struct Abort_repl	*reply;
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	uint32_t		size;
	TALLOC_CTX		*mem_ctx;
	mapi_ctx_t		*mapi_ctx;

	/* Sanity checks */
	MAPI_RETVAL_IF(!global_mapi_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!obj_table, MAPI_E_INVALID_PARAMETER, NULL);
	MAPI_RETVAL_IF(!TableStatus, MAPI_E_INVALID_PARAMETER, NULL);

	mapi_ctx = global_mapi_ctx;
	mem_ctx = talloc_init("Abort");

	size = 0;

	/* Fill the MAPI_REQ request */
	mapi_req = talloc_zero(mem_ctx, struct EcDoRpc_MAPI_REQ);
	mapi_req->opnum = op_MAPI_Abort;
	mapi_req->logon_id = 0;
	mapi_req->handle_idx = 0;
	size += 5;

	/* Fill the mapi_request stucture */
	mapi_request = talloc_zero(mem_ctx, struct mapi_request);
	mapi_request->mapi_len = size + sizeof (uint32_t);
	mapi_request->length = size;
	mapi_request->mapi_req = mapi_req;
	mapi_request->handles = talloc_array(mem_ctx, uint32_t, 1);
	mapi_request->handles[0] = mapi_object_get_handle(obj_table);

	status = emsmdb_transaction(mapi_ctx->session->emsmdb->ctx, mapi_request, &mapi_response);
	MAPI_RETVAL_IF(!NT_STATUS_IS_OK(status), MAPI_E_CALL_FAILED, mem_ctx);
	retval = mapi_response->mapi_repl->error_code;
	MAPI_RETVAL_IF(retval, retval, mem_ctx);

	/* Retrieve TableStatus */
	reply = &mapi_response->mapi_repl->u.mapi_Abort;
	*TableStatus = reply->TableStatus;

	return MAPI_E_SUCCESS;
}
