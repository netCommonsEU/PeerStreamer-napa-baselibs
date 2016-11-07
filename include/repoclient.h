#ifndef _REPOCLIENT_H
#define _REPOCLIENT_H

/** @file repoclient.h
 * 
 * Client-side interface for the Repository.
 *
 * Peers access the Repository for querying measurement results (PeerIDs satisfying a given query)
 * and publish MeasurementRecords generated by their Monitoring modules.
 * The following data structures and function definitions facilitate this (client-side) access to a repository.
 * 
 * Overall considerations:
	-# In the prototype, the Repository operates as a cental network resource (server). However, peers
may execute several repoclient instances, and connect to several Repository servers at the same time.
and has its own executable program, to be run independent of other NAPA processes. 
	-# The repository controller uses an internal network protocol for querying and publishing (inserting) measurement records, and this API defines the client side of this protocol.
	-# The following kinds of queries are supported:
		-# Metadata query: list of available (or implemented/supported) 
measurement kinds (measurementIDs)
		-# Peer-Query: resulting a list of peers, satisfying the supplied condition 
on measurement values, ordered by a supplied linear function over measurements
		-# Measurement value query: returning measurement record(s) for a given PeerID 
	-# The design of the repository client API follows the reactive, event-based pattern: function calls are
	aynchronous, i.e. the caller passes a <i>callback</i> function which is called when the result becomes 
	available. Consequently, all calls are non-blocking and return immediately.
 *
 * <b>Repository Protocol Description</b>
 * 
 * Internally, the Repository uses a HTTP-based client-server network protocol.
 * All protocol operations are text-encoded as HTTP GET operations.
 * Additionally, there is a HTTP POST based interface for PUBLISH operations where batch publish requests can be sent.
 * Clients connect to Respository servers over a well-known TCP port, and issue HTTP requests as described below.
 * Operation success or failure is indicated with a standard HTTP error code, 200 (HTTP OK) for success, or a standard
 * HTTP error code such as 403 (Bad Request), 404 (Not Found) or 500 (Internal Server Error).
 *
 * Repository server answers (if any) are always of MIME type "plain/text".
 *
 * Protocol elements and encodings:
 	- <b>Pubish</b> Used to publish measurement results (a MeasurementRecord) into the Repository.
		- Encoding: <tt>http://reposerver:port/Publish?originator=o&targetA=a&targetB=b&publish_name=n&value=v&string_value=s&channel=c&timestamp=ts</tt><br>
		where each argument represents the corresponding field from MeasurementRecord. 
		All values are interpreted as strings, except <i>value</i> (double) and <i>timestamp</i>, which is
		encoded by one of the following encodings:
			- an integer (seconds since 1970-01-01 00:00:00) 
			- using the format YYYY-MM-DD_HH24:mm:ss 
			- using the word <tt>now</tt><br> (default if timestamp is missing)
		Mandatory arguments are: <i>originator</i>, <i>targetA</i>, <i>publish_name</i>. 
		Omitted values default to NULL (or NaN for <i>value</i>
		Alternative (HTTP POST) encoding: <tt>http://reposerver:port/BatchPublish</tt>
		The POST DATA contains text-encoded MeasurementRecords of the same format as used with the GET
		method (i.e. starting with originator=o...)
		- Returns: a HTTP error code, 200 for OK.
	- <b>ListMeasurementNames</b> is used to obtain a list of measurement names the Repository has data for.
		- Encoding: <tt>http://reposerver:port/ListMeasurementNames?maxresults=m&channel=c</tt><br>
		- Returns: a HTTP error code, 200 for OK and the textual names of MeasurementNames, one item per line.
		  At most <i>m</i> items are returned ((unlimited if m <= 0, which is the default if m is omitted).
	- <b>GetPeers</b> is used to obtain a list of peers satisfying given criteria. 
		- Encoding: <tt>http://reposerver:port/GetPeers?maxresults=m&constraints=mtype,min,max;mtype,min,max;...&ranking=mtype,w;mtype,w...&channel=c</tt><br>
		<i>constraints</i> and <i>ranking</i> tuples correspond to Constraint and Ranking specifications 
		(each tuple is separated by a semicolon), and <i>maxresults</i>
		specifies the maximum number of peers to return (unlimited if m <= 0, which is the default if m is omitted).
		All parameters are optional. If no Ranking is specified, the results are returned in random order.
		- Returns: a HTTP error code, 200 for OK and the PeerIDs of the peers found, one item per line.
	- <b>CountPeers</b> Similar to GetPeers, just returns the number of matching PeerIDs
		- Encoding: <tt>http://reposerver:port/CountPeers?maxresults=n&c=mtype,min,max;...&r=mtype,w;...&channel=c</tt><br>
		- Returns: a HTTP error code, 200 for OK and the number PeerIDs (a single integer followed by a newline)
	- <b>GetMeasurements</b> Used to obtain entire measurement records
		- Encoding: <tt>http://reposerver:port/GetMeasurements?maxresults=m&originator=o&targetA=a&targetB=b&publish_name=n&channel=c</tt><br>
		where all parameters are optional and function as filters, except for <i>maxresults</i>, which has the
		same meaning as in the above functions.
		- Returns: a HTTP error code, 200 for OK and the MeasurementRecords found, one item per line,
		in the same format what is used for the Publish call.  Missing (NULL-valued) fields are omitted.
*/

#include	"napa.h"
#include	"mon.h"

/** Constraints are used in GetPeer and CountPeer functions to specify matching criteria for numeric-value MeasurementRecords */
typedef struct {
	/** Measurement identifier */
	char *published_name;
	/** String equality value. If not NULL, minValue and maxValue are ignored */
	char *strValue;
	/** Minimum value */
	double minValue;
	/** Maximum value */
	double maxValue;
} Constraint;

/** Ranking is used to order the result set of GetPeer queries. */
typedef struct {
	/** Ranking measurement identifier */
	char *published_name;
	/** Ranking measurement weight */
	double weight;
} Ranking;

/** Callback for returning the result of repListMeasurementNames call.

  @param rep handle of the repository client.
  @param id handle of the particular request (assigned by the repListMeasurementNames call)
  @param cbarg arbitrary user-provided parameter for the callback
  @param result result array, to be freed by the callback.
  @param nResults number of entries in result
  @see repListMeasurementNames
*/
typedef void (*cb_repListMeasurementNames)(HANDLE rep, HANDLE id, void *cbarg, char **result, int nResults);

/** Callback for returning the result of repGetPeers call.

  @param rep handle of the repository client.
  @param id handle of the particular request (assigned by the repGetPeers call)
  @param cbarg arbitrary user-provided parameter for the callback
  @param result result array, to be freed by the callback.
  @param nResults number of entries in result
  @see repGetPeers 
*/
typedef void (*cb_repGetPeers)(HANDLE rep, HANDLE id, void *cbarg, char **result, int nResults);

/** Callback for returning the result of repCountPeers call.

  @param rep handle of the repository client.
  @param id handle of the particular request (assigned by the repCountPeers call)
  @param cbarg arbitrary user-provided parameter for the callback
  @param nPeers result
  @see repCountPeers 
*/
typedef void (*cb_repCountPeers)(HANDLE rep, HANDLE id, void *cbarg, int nPeers);

/** Callback for returning the results of a repGetMeasurements call.

  @param rep handle of the repository client.
  @param id handle of the particular request (assigned by the repGetMeasurements call)
  @param cbarg arbitrary user-provided parameter for the callback
  @param result result array, to be freed by the callback.
  @param nResults number of entries in result
  @see repGetMeasurements
*/
typedef void (*cb_repGetMeasurements)(HANDLE rep, HANDLE id, void *cbarg, MeasurementRecord *result, int nResults);

/** Callback for informing about the completion of a publish call 

  @param rep handle of the repository client.
  @param id handle of the particular request (assigned by the publish call)
  @param cbarg arbitrary user-provided parameter for the callback
  @param result result code, 0 on success
  @see repPublish 
*/
typedef void (*cb_repPublish)(HANDLE rep, HANDLE id, void *cbarg, int result);

#ifdef __cplusplus
extern "C" {
#endif
/**
  Initialize the repository client library.

  Needs to be called upon bootstapping the application (prior to the first repOpen call)

  @param config the relevant configuration file section
*/
void repInit(const char *config);

/**
  Open a repository client instance to a given repository server.

  @param server repository server address in addr:port format, e.g. 192.168.1.1:9832 or myserver.org:1234
  @param publish_period batch publish accumulated records this often. If 0, publish each record immediately.
  @return HANDLE if successful, NULL on error
  @see repClose
*/
HANDLE repOpen(const char *server, int publish_period);

/**
  Close the repository client instance.

  Closes the client instance and frees allocated resources.
  @param repoclient the instance to be closed.
  @see repInit
*/
void repClose(HANDLE repoclient);

/**
  Query the list of available(stored) MeasurementNames.

  Use this function to list what MeasurementNames are stored by this repository.

  @param rep the repository instance to be queried
  @param cb callback for returning the results
  @param cbarg arbitrary user-provided parameter for the callback
  @param maxResults  return at most maxResults MeasurementNames
  @param ch channel filter, can be NULL for any channel
  @return the id for identifying the result in the callback or NULL on error
*/
HANDLE repListMeasurementNames(HANDLE rep, cb_repListMeasurementNames cb, void *cbarg, int maxResults, const char *ch);

/** 
  Query the repository for a list of Peers satisfying given criteria.

  This is the main use case for the repository client: use this function to obtain a list of peers, where
  each peer satisfies the following:
	- There are measurement records available for it of the MeasurementName s specified in cons
	- The values of these measurements are within the range specified by the constraints
  The resulting list is ordered by a linear function composed of the Ranking list (each measurement value
  is multiplied by the weight assigned to the particular MeasurementName and these numbers are summed for
  each peer.

  @param rep the repository instance to be queried
  @param cb callback for returning the results
  @param cbarg arbitrary user-provided parameter for the callback
  @param maxPeers return at most n PeerIDs
  @param cons array of Constraints
  @param clen length of cons array
  @param ranks  array of Rankings
  @param rlen length of ranks array
  @param ch channel filter, can be NULL for any channel
  @return the id for identifying the result in the callback or NULL on error
  @see repCountPeers
*/
HANDLE repGetPeers(HANDLE rep,cb_repGetPeers cb, void *cbarg, int maxPeers, Constraint *cons, int clen, Ranking *ranks, int rlen, const char *ch);

/** 
  Count the number of Peers satisfying given criteria.

  This function is similar to repGetPeers, just instead of returning the actual list of Peers, only counts them.

  @param rep the repository instance to be queried
  @param cb callback for returning the result
  @param cbarg arbitrary user-provided parameter for the callback
  @param cons array of Constraints
  @param clen length of cons array
  @param ch channel filter, can be NULL for any channel
  @return the id for identifying the result in the callback or NULL on error
*/
HANDLE repCountPeers(HANDLE rep, cb_repCountPeers cb, void *cbarg, Constraint *cons, int clen, const char *ch);

/** 
  Publish a measurementrecord to the given repository.

  The monitoring layer uses this function to publish measurement records to the Repository.
  @param rep the repository instance to be queried
  @param cb callback for returning the result (optional, NULL if not needed)
  @param cbarg arbitrary user-provided parameter for the callback
  @param r the record containing measurement values.
  @return the id for identifying the result in the callback or NULL on error
*/
HANDLE repPublish(HANDLE rep, cb_repPublish cb, void *cbarg, MeasurementRecord *r);

/**
  Get MeasurementRecord entries from the Repository.

  @param rep the repository instance to be queried
  @param cb callback for returning the result (optional, NULL if not needed)
  @param cbarg arbitrary user-provided parameter for the callback
  @param maxResults maximum number of MeasurementRecords to return (unlimited if <= 0)
  @param originator originator filter
  @param targetA targetA filter
  @param id MeasurementName filter
  @param channel channel filter, can be NULL for any channel
  @return the id for identifying the result in the callback or NULL on error
*/
HANDLE repGetMeasurements(HANDLE rep, cb_repGetMeasurements cb, void *cbarg, int maxResults, const char *originator, const char *targetA, const char *targetB, const char *measurementName, const char *channel);

/** 
  Helper function for debugging.

  @param r the record to be printed
  @result Textual format in a statically allocated buffer.
*/
const char *measurementrecord2str(const MeasurementRecord r);

#ifdef __cplusplus
}
#endif

#endif

