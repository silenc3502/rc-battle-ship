/** ===========================================================================
 * @file exp_pp_h265decoder.h
 *
 * @brief This file declares API functions and data structures required for
 *        H265 decoder
 *
 * ============================================================================
 *
 * Copyright © PathPartner Technology, 2012-2015
 *
 * This material,including documentation and  any related computer programs,is
 * protected by copyright controlled by PathPartner Technology. All rights are
 * reserved.
 *
 * ============================================================================
 *
 * <b> REVISION HISTORY </b>
 * @n  ================  @n
 * @Version 0.1 : 30-Jun-2012 : Prasad RSV : Initial Code
 * @Version 0.2 : 19-Jul-2012 : Sunil K : Fixed bug for memory manager
 * $RevLog$
 *
 *=============================================================================
 */

#ifndef _EXP_PP_H265DECODER_H_
#define _EXP_PP_H265DECODER_H_

/** @defgroup H265_DECODER
*/

/** Define to indicate FALSE
*/
#define PP_FALSE    0
/** Define to indicate TRUE
*/
#define PP_TRUE     1

/** Basic data type to store unsigned integer of 8 bits
*/
typedef unsigned char    tPPu8;
/** Basic data type to store unsigned integer of 16 bits
*/
typedef unsigned short   tPPu16;
/** Basic data type to store unsigned integer of 32 bits
*/
typedef unsigned int     tPPu32;

/** Basic data type to store signed integer of 8 bits
*/
typedef signed char      tPPi8;
/** Basic data type to store signed integer of 16 bits
*/
typedef signed short    tPPi16;
/** Basic data type to store signed integer of 32 bits
*/
typedef signed int      tPPi32;
/** Basic data type to store unsigned integer of 64 bits
*/
typedef unsigned long long  tPPu64;

/** Basic data type to store signed integer of 64 bits
*/
typedef signed long long  tPPi64;

/** Basic data type to store 32 bit fractional values
*/
typedef float           tPPf32;
/** Basic data type to store 64 bits fractional values
*/
typedef double          tPPf64;

/** Data type of return value of functions
All the funtions must return this data type for consistency across all modules
*/
typedef signed int      tPPResult;
/** Bool data type
Any of PP_TRUE or PP_FALSE values
*/
typedef unsigned char   tPPBool;
/** Flag data type
Any of PP_ON or PP_PFF values
*/
typedef unsigned char   tPPFlag;

/*Status codes*/

/** Success
*/
#define SC_PP_SUCCESS                 (0)

/** API has successfully accepted the request , however its action asyhchornous
*/
#define SC_PP_PENDING                 (1)

/** End Of Picture
*/
#define SC_PP_END_OF_PICTURE          (2)

/** Error Occured while parsing
*/
#define SC_PP_PARSING_ERROR           (3)

/** Slice of different Access Unit found before completion of current AU
*/
#define SC_PP_NEW_AU_FOUND            (4)
/** slice data error
*/
#define SC_PP_SLICE_DATA_ERR          (5)
/** Reference not found
*/
#define SC_PP_REFPIC_NOT_FOUND        (6)

/** Picture resolution changed
*/
#define SC_PP_RESOLUTION_CHANGED      (7)
/** Decoding successful but complete frame is not yet decoded
*  Successfully decoded a packet data
*  but more packets required for complete frame decoding.
*/
#define SC_PP_DECODE_INCOMPLETE       (8)

/***************************************************************
                        General Error codes
***************************************************************/
#define _EC_PP_GENERAL_ERROR_BASE_    (0)
/** General Failure
Cause is unknown.
*/
#define EC_PP_FAILURE               (_EC_PP_GENERAL_ERROR_BASE_-1)
/** Failure due to low memory condition.
System might be running on low memory hence allocation failed.
*/
#define EC_PP_OUT_OF_MEMORY         (_EC_PP_GENERAL_ERROR_BASE_-2)
/** Failure due to invalid parameter passed.
*/
#define EC_PP_INVALID_PARAM         (_EC_PP_GENERAL_ERROR_BASE_-3)
/** Operation is Not supported
*/
#define EC_PP_NOT_SUPPORTED         (_EC_PP_GENERAL_ERROR_BASE_-4)
/** Buffer passed is insufficient to complete the operation.
*/
#define EC_PP_INSUFFICIENT_BUFFER   (_EC_PP_GENERAL_ERROR_BASE_-5)
/** Operation failed due memory corruption
*/
#define EC_PP_CORRUPT_DATA          (_EC_PP_GENERAL_ERROR_BASE_-6)


/***************************************************************
File/directory operations related return codes.
***************************************************************/
#define _EC_PP_FILE_OPERATION_ERROR_BASE_ (-100)
/** File open failed.
Either the file is not present or
the file is locked by other application in exclusive mode.
*/
#define EC_PP_FILE_OPEN_FAIL      (_EC_PP_FILE_OPERATION_ERROR_BASE_ - 1)
/** File close failed.
*/
#define EC_PP_FILE_CLOSE_FAIL     (_EC_PP_FILE_OPERATION_ERROR_BASE_ - 2)
/** File read failed.
File pointer might be at the end of the file.
*/
#define EC_PP_FILE_READ_FAIL      (_EC_PP_FILE_OPERATION_ERROR_BASE_ - 3)
/** File write operation failed.
*/
#define EC_PP_FILE_WRITE_FAIL     (_EC_PP_FILE_OPERATION_ERROR_BASE_ - 4)
/** File seek operation failed.
*/

/** End of file reached
*/
#define EC_PP_END_OF_FILE         (_EC_PP_FILE_OPERATION_ERROR_BASE_ - 5)

#define EC_PP_H265_STATUSCODE_BASE (-1000)

/** No Sync word found in the given stream
*/
#define EC_PP_H265_NO_SYNC_FOUND (EC_PP_H265_STATUSCODE_BASE - 1)
/** No end Sync word found in the given stream - indicating insuffucient
    buffer to detect second sync word => end of present NALU
*/
#define EC_PP_H265_NO_NALU_END_FOUND (EC_PP_H265_STATUSCODE_BASE - 2)

/** Forbidden bit should be zero for all Nal unit - if its non-zero
    it indicates corruption of bit-stream
*/
#define EC_PP_H265_FORBIDDEN_BIT_NON_ZERO (EC_PP_H265_STATUSCODE_BASE - 3)

/** If NAL unit type is not identified.
*/
#define EC_PP_H265_INVALID_NAL_UNIT_TYPE (EC_PP_H265_STATUSCODE_BASE - 4)

/** Invalid combination of NAL Unit type and reference IDC
*/
#define EC_PP_H265_INVALID_NAL_UNIT_TYPE_AND_REF_IDC \
                                       (EC_PP_H265_STATUSCODE_BASE - 5)
/** No start Sync word found in the given stream - indicating insuffucient
    buffer to detect start sync word => start of present NALU
*/
#define EC_PP_H265_NO_NALU_START_FOUND   (EC_PP_H265_STATUSCODE_BASE - 6)

/** Maximum reference frames
* Maximum reference frames this decoder configuration supports.
* @ingroup H265_DECODER
*/
#define FILE_NAME_SIZE 512

#define PP_H265VDEC_MAX_REF_FRAMES 16

/** Set status parameters macros*/
#define PP_SET_DATASYNC             1
#define PP_SET_DPB_FLUSH            2
#define PP_SET_RES_INV              3
#define PP_SET_DECODE_HDR           4

/** Get status parameters macros*/
#define PP_GET_PARAMSTATUS          1
#define PP_GET_BUFSTATUS            2
#define PP_GET_ERRORSTATUS          3

/** Pic width and height*/
#define PP_H265_MAX_PIC_WIDTH        4096
#define PP_H265_MAX_PIC_HEIGHT       2176

/**************      SEI & VUI Related Macros and Structures     **************/

/* SEI related definition */
/**
 *  @brief SEI types
 */
typedef enum
{
  H265_SEI_BUFFERING_PERIOD,
  H265_SEI_PIC_TIMING,
  H265_SEI_PAN_SCAN_RECT,
  H265_SEI_FILLER_PAYLOAD,
  H265_SEI_USERDATA_REGISTERED_ITU_T_T_35,
  H265_SEI_USERDATA_UNREGISTERED,
  H265_SEI_RECOVERY_POINT,
  H265_SEI_SCENE_INFO = 9,
  H265_SEI_PICTURE_SNAPSHOT = 15,
  H265_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  H265_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  H265_SEI_FILM_GRAIN_CHARACTERISTICS = 19,
  H265_SEI_POST_FILTER_HINT = 22,
  H265_SEI_TONE_MAPPING_INFO,
  H265_SEI_FRAME_PACKING_ARRANGEMENT = 45,
  H265_SEI_DISPLAY_ORIENTATION = 47,
  H265_SEI_STRUCTURE_OF_PICTURES_INFO = 128,
  H265_SEI_ACTIVE_PARAMETER_SETS,
  H265_SEI_DECODING_UNIT_INFO,
  H265_SEI_TEMPORAL_SUB_LAYER_ZERO_INDEX,
  H265_SEI_DECODED_PICTURE_HASH,
  H265_SEI_SCALABLE_NESTING,
  H265_SEI_REGION_REFRESH_INFO,
  H265_SEI_RESERVED
}H265_SEIType;

/* SEI PayLoad Types */
#define H265_SEI_PICTURE_DIGEST            256
#define H265_MAX_SEI_USERDATA              256

/* Max value of (cpb_cnt_minus1 + 1) */
#define H265_MAX_CPB_CNT                   32
#define H265_MAX_PAN_SCAN_RECT             3

#define     H265_MAX_NUM_PIVOTS       65535  /* (1 << bitdepth) */
/* Max Video PLane Buffer 3 (Y,Cb,Cr) */
#define     H265_MAX_NUM_VIDEOPLANE_BUF  3
/* Max Meta Data Buffer currently 2 (SEI & VUI) */
#define     H265_MAX_NUM_METADATA_BUF  2
/* SEI & VUI Related MACROS */
#define     H265_SEI_TOTAL_COL_COMP             3
#define     H265_SEI_EXTENDED_ISO               255
#define     H265_SEI_EXTENDED_SAR               255
#define     H265_SEI_UUID_ISO_IEC_128           16
#define     H265_SEI_INTENSITY_LEVEL_INTERVAL   256
#define     H265_SEI_MAX_NUM_NESTING_OPS        1024
#define     H265_SEI_MAX_NUM_NESTING_LAYER      64
#define     H265_SEI_COL_COMP_MAX_MODEL_ID      8
#define     H265_SEI_COL_FILTER_HINT_X          15
#define     H265_SEI_COL_FILTER_HINT_Y          15
#define     H265_SEI_MAX_NUM_SOP                1024
#define     H265_SEI_MAX_MD5_DATA_LEN           16
#define     H265_SEI_BIT_DEPTH                  16 /* upto 16 supported */
#define     H265_HRD_MAX_CPB_COUNT              32
/* need to check ,changed from 6 to 8 */
#define     H265_HRD_MAX_NUM_SUB_LAYER          8

/* supported Macros required */
#define     PP_H265_MIN_LCU_SIZE    16
#define     PP_H265_MAX_NUM_LCU_COLUMNS  \
    (PP_H265_MAX_PIC_WIDTH/PP_H265_MIN_LCU_SIZE)
#define     PP_H265_MAX_NUM_LCU_ROWS     \
    (PP_H265_MAX_PIC_HEIGHT/PP_H265_MIN_LCU_SIZE)
#define     PP_H265_MAX_NUM_LCU          \
                         (PP_H265_MAX_NUM_LCU_COLUMNS * \
                         PP_H265_MAX_NUM_LCU_ROWS)


#ifdef __cplusplus
extern "C"
{
#endif
/** Metadata macros*/
#define PP_METADATAPLANE_NONE       -1
#define PP_MAX_NUM_METADATA_PLANES  3
/** Reference frame macros */
#define PP_MAX_REF_FRAME_COUNT 18
#define PP_MAX_INBUF_COUNT 5


/** bitstream structure
* @ingroup BASE_CODEC
* Structure to contain encoded bitstream.
*/
typedef struct PPInput_BitStream
{
    tPPu8 *nBitStream; /* Buffer to hold the encoded bit stream */
    tPPi32 nBufLength; /* Contains the length of bit Stream buffer in bytes */
}tPPInput_BitStream;

/** Frame object identifying the input buffer element
* @ingroup BASE_CODEC
* Contains identification information to tag the input buffer submitted to the
* decode function. This identification tag will be used by the decoder to mark
* buffers for display and or freeing.
*/
typedef struct PPInFrame_Buff
{
 /*! Unique Id which identifies the buffer element */
    tPPi32 bufId;
    tPPu8 *buf[PP_MAX_INBUF_COUNT];
}tPPInFrame_Buff;

/** list of buffer elements to be freeded up.
* @ingroup BASE_CODEC
* Contains list of identification tags of buffers which the decoder intends to
* free-up and there by returning them back to buffer managers pool.
*/
typedef struct PPOutFrame_Buff
{
 /* Unique Id which identifies the buffer element */
 /* the elements in the list are to be freed as its freed by decoder*/
    tPPi32 bufId[PP_MAX_REF_FRAME_COUNT];
}tPPOutFrame_Buff;

/** Memory requirement Query
* Structure used to query for memory requirements of the codec object
* codec create function calculates exact
* @ingroup BASE_CODEC
*/
typedef struct PPQueryMemRecords
{
    tPPu8 *allotedHandle; /* contains alloted handle to be given to
                          initialise handle */
    tPPi32 reqSize;       /* Size requested in bytes */
    tPPi32 memType;      /* Type of memory(Scratch or Persistance)*/
}tPPQueryMemRecords;

typedef enum {
  kH265_TASK_MASTER         = 0,   /** Global master       */
  kH265_TASK_SLAVE          = 1    /** Slave cores         */
} eH265_multiThreadTask;

/** Multi-thread
* Structure used to hold all multi-thread parameteres
* @ingroup BASE_CODEC
*/
typedef struct PPMultiThreadParams
{
    tPPi32 nThreadID;       /* Thread ID */
    eH265_multiThreadTask nTaskID;  /* Core task Identification:
                                    Reserved MASTER_CORE */
    tPPi32 nNumThreads;   /* Num of Threads */
}tPPMultiThreadParams;


typedef struct ppH265Dec_CreateParams tPP_H265_CreateParams;
/** Create parameter structure for decoder
 *  @brief Structure containing attributes of the decoder object to be instanced
 *  @ingroup H265_DECODER
 */
struct ppH265Dec_CreateParams
{
    /* Max width*/
    tPPi32  nMaxWidth;
    /* Max height */
    tPPi32  nMaxHeight;
    /* Number of CPUs */
    tPPi32  nNumThreads;
    /* Inputdata sync enable flag */
    tPPi32 nInputLowDelayMode;
    /* Output data sync enable flag */
    tPPi32 nOutputLowDelayMode;
    /* number of CTU rows */
    tPPi32 nNumCTURows;
    /* Metadata type - SEI , VUI*/
    tPPi32 nMetadataType;
};
/**
 *  @brief          Descriptor for the chunk of data being
 *                  transferred in one call to putData/Get data
 */
typedef struct H265DataSyncDesc tPP_H265_DataSyncDesc;

struct H265DataSyncDesc
{
    tPPi32 nNumBlocks;           /**< Number of blocks available */
    tPPi32 nSize;                /**< sizeField */
};

/**
 *  @brief      Non-blocking API to signal "data ready" to one or more
 *              consumers.
 *  @param[out] pDataSyncDesc    Full data sync descriptor.  This includes one
 *                              or more filled data buffers.
 *
 */
typedef void (*gH265_DataSyncPutFxn)(tPP_H265_DataSyncDesc *pDataSyncDesc);
/**
 *  @brief      Non-blocking API to signal "data ready" to one or more
 *              consumers.
 *  @param[out] pDataSyncDesc    Full data sync descriptor.  This includes one
 *                              or more filled data buffers.
 *
 */
typedef void (*gH265_DataSyncGetFxn)(tPP_H265_DataSyncDesc *pDataSyncDesc);

typedef struct  ppH265Dec_DynamicParams tPP_H265_DynamicParams;
/** Dynamic parameter structure for decoder
 *  @brief Structure containing attributes of the decoder that
 *  can change dynamically
 *  @ingroup H265_DECODER
 */
struct ppH265Dec_DynamicParams
{
     tPPi32 nDecodeHeader; /*parameter for header decoder */
     gH265_DataSyncPutFxn     fOutputLowDelayFxn;
                        /**< Optional datasync "put data" function.
                         *
                         *   @remarks   Apps/frameworks that don't
                         *              support datasync should set
                         *              this to NULL.
                         *
                         *   @remarks   This function is provided
                         *              by the app/framework to the
                         *              video decoder.  The decoder
                         *              calls this function when
                         *              sub-frame data has been put
                         *              into an output buffer and is
                         *              available.
                         */
     gH265_DataSyncGetFxn   fInputLowDelayFxn;
                        /**< Optional datasync "get data" function.
                         *
                         *   @remarks   Apps/frameworks that don't
                         *              support datasync should set
                         *              this to NULL.
                         *   @remarks   This function is provided
                         *              by the app/framework to the
                         *              video decoder.  The decoder
                         *              calls this function to get
                         *              partial compressed bit-stream
                         *              data from the app/framework.
                         */
};
/** Status parameter structure from/for decoder
 *  @brief Structure containing attributes of the decoder object instanced.
 *  this structure is also used to change the attributes of the decoder.
 *  @ingroup H265_DECODER
 */
typedef struct ppH265Dec_StatusParams
{
  /** Frame size */
  tPPi32 nTotalFrameSize;
  /** Pic width */
  tPPi32 nPicWidth;
  /** Pic height */
  tPPi32 nPicHeight;
  /** Bitstream profile*/
  tPPi32  nProfile;
  /* Input Low delay mode */
  tPPi32 nInputLowDelayMode;
  /* Output Low delay mode */
  tPPi32 nOutputLowDelayMode;
  /* Function pointer for input low delay*/
  gH265_DataSyncGetFxn     fInputLowDelayFxn;
  /* Function pointer for output low delay*/
  gH265_DataSyncPutFxn     fOutputLowDelayFxn;
  /* number of CTU rows */
  tPPi32 nNumCTURows;
  /* Metadata type - SEI , VUI*/
  tPPi32 nMetadataType;
  /*  Header decode status*/
  tPPi32 nDecodeHeader;
  /* Error status */
  tPPi32  nError;

}tPPDecParam_Status;

/** Base video frame structure
This structure defines the base video frame.
It contains the frame size information and also
padding information
*/
typedef struct ppBaseVideoFrame
{
    tPPi32              nWidth;     /**< Actual width of the frame */
    tPPi32              nHeight;    /**< Actual height of the frame */
    tPPi32              nIsPadded;  /**< Flag to indicate if the frame is
                                    padded*/
    tPPi32              nExWidth;   /**< Extended width of the frame due to
                                    padding*/
}tPPBaseVideoFrame;


/** YUV Planar video frame
YUV planar frame shall have seperate lum, cb and cr data.
*/
typedef struct
{
    tPPBaseVideoFrame nBaseFrame;   /**< Derived from base video frame */
    tPPu8       *pLum;              /**< Lum data pointer   */
    tPPu8       *pCb;               /**< Cb data pointer    */
    tPPu8       *pCr;               /**< Cr data pointer    */
}tPPYUVPlanarDisplayFrame;

/** Base Decoder definition
* @ingroup BASE_DECODER
*/
typedef struct ppBaseDecoder tPPBaseDecoder;
/** Base Decoder definition
* @ingroup BASE_DECODER
*/
struct ppBaseDecoder
{

/**
******************************************************************************
* tPPResult (*vDecode) (tPPBaseDecoder    *apBase,
*                       tPPInput_BitStream *apInBitStream,
*                       tPPInFrame_Buff     *apH265_dec_InBuff,
*                       tPPOutFrame_Buff    *apH265_dec_OutBuff,
*                       tPPYUVPlanarDisplayFrame *apFrame,
*                       tPPMultiThreadParams *nMTParam)
*
*
* @brief This virtual interface decodes a bitstream to produce a output raw
* frame. The input bitstream can be packet mode or frame mode. Handling of
* both modes is done differently as follows:
* Frame Mode:
*   In this mode, when the API is able to decode the frame successfully, it
*   returns SC_PP_SUCCESS and returns the output frame in apFrame ( if not
*   NULL).
*
* Packet Mode:
*
*   if the packets are decoded in the order,and if the decoder is able to
*   detect the last packet of the frame by any means, then this API shall
*   return SC_PP_SUCCESS and returns the output frame in apFrame (if not NULL)
*   when it successfully decodes the last packet of the frame.
*
*   If the packets are not decoded in correct order, the decoder shall detect
*   the end of frame decoding only when it receives the packet belonging to
*   the next frame. In this scenario, when the packet belonging to next frame
*   is passed for decoding , the decoder must not consume the input bitstream,
*   but must return SC_PP_SUCCESS to indicate the successful decode of the
*   last frame and return the output of the last frame in apFrame parameter
*   (if not NULL). The caller must call the decode API again with the previous
*   unconsumed bitstream (containing packet of the next frame).
*
* @param apBase              [IN] Pointer to base decoder interface
* @param apBitStream         [IN] Bit stream structure whicha has data to be
*                                 decoded.
* @param apH265_dec_InBuff   [IN] This holds the pointer to the buffer
*                                 allocated by the buffer manager to the
*                                 decoder.The decoder stores the current
*                                 decoded frame in this location.
*                                 Length of the bitstream.
* @param apH265_dec_OutBuff  [OUT] This holds the pointer to the buffer to be
*                                   freed by the decoder. The application sets
*                                   "0" in these locations as BuffID's. The
*                                   decoder sets the BuffID of the buffer which
*                                   needs to be freeds
* @param apFrame             [IN/OUT] The decoded frame output only if the
*                                      return value is SC_PP_SUCCESS . The
*                                      structure that holds the array of frames
*                                      to be displayed.If this parameter is
*                                      NULL, the output frame MAY be queried
*                                      through the vGet interface.
*
* @param nMTParam            [IN]      Multi thread parameters
* @return Status code
*
*      SC_PP_SUCCESS             : Successfully decoded the complete frame
*                                  (in frame mode) the apFrame shall contain
*                                   the decoded frame data OR successfully
*                                  decoded the last packet of the frame
*                                  (in packet mode) OR detected successfull
*                                  decoding of frame data when received packet
*                                  of next frame ( in packet mode). The
*                                  apFrame shall contain the decoded frame data
*      SC_PP_DECODE_INCOMPLETE   : Only applicable in packet mode. Successfully
*                                  decoded a packet data but more packets
*                                  required for complete frame decoding. The
*                                  apFrame shall not contain any valid frame
*                                  data.
*      EC_PP_FAILURE             : General failure
*
*      EC_PP_INVALID_PARAM       : Failed due to incorrect params.
*                                  (or NULL params/handle)
*      EC_PP_CORRUPT_DATA        : Error encountered during decoding due to
*                                  corrupt data
*      EC_PP_H265_NO_NALU_END_FOUND : Finds no NAL unit end
*
*      SC_PP_RESOLUTION_CHANGED  :  Resolution got Changed.
*
*     EC_PP_INSUFFICIENT_BUFFER  :  Bit Stream End Found.
*
******************************************************************************
*/
tPPResult (*vDecode)(tPPBaseDecoder            *apBase,
                     tPPInput_BitStream        *apInBitStream,
                     tPPInFrame_Buff           *apH265_dec_InBuff,
                     tPPOutFrame_Buff          *apH265_dec_OutBuff,
                     tPPYUVPlanarDisplayFrame  *apFrame,
                     tPPMultiThreadParams *nMTParam);

/**
 ******************************************************************************
 * @fn tPPResult vReset( tPPBaseDecoder *apBase);
 *
 * @brief This virtual interface resets the decoder.
 *
 * @param apBase  [IN] Pointer to base decoder interface

 *
 * @return Status code SC_PP_SUCCESS if success, or the following error codes
 *  EC_PP_FAILURE : General failure
 *
 * @ingroup BASE_DECODER
 ******************************************************************************
 */
       tPPResult (*vReset)( tPPBaseDecoder *apBase);

/**
 ******************************************************************************
 * @fn  tPPResult vSet( tPPBaseDecoder *apBase, tPPu32 anFlag, tPPi32 anVal);
 *
 * @brief This virtual interface to set some parameters to the decoder.
 *
 * @param apBase  [IN] Pointer to base decoder interface
 * @param anFlag  [IN] Flag to specify the parameter which needs to be set
 * @param anVal   [IN] Value of the parameter
 * @param apDynamicParams [IN] Dynamic parameter handle
 *
 * @return Status code SC_PP_SUCCESS if success, or the following error codes
 *  EC_PP_FAILURE : General failure
 *
 * @ingroup BASE_DECODER
 ******************************************************************************
 */
       tPPResult (*vSet)( tPPBaseDecoder *apBase, tPPu32 anFlag, tPPi32 anVal,
           tPP_H265_DynamicParams *apDynamicParams);



/**
 ******************************************************************************
 * @fn  tPPResult vGet( tPPBaseDecoder *apBase, tPPu32 anFlag, void* apVal);
 *
 * @brief This virtual interface to get some parameters of the decoder.
 *
 * @param apBase    [IN] Pointer to base decoder interface
 * @param anFlag    [IN] Flag to specify the parameter which needs to be get
 * @param anVal     [IN/OUT] Pointer to hold the value of the parameter

 *
 * @return Status code SC_PP_SUCCESS if success, or the following error codes
 *  EC_PP_FAILURE : General failure
 *
 * @ingroup BASE_DECODER
 ******************************************************************************
 */
 tPPResult (*vGet)( tPPBaseDecoder *apBase, tPPu32 anFlag, void *apVal);

};

/*enum for Memory type*/
typedef enum
{
    PP_H265_PERSIST = 0,
    PP_H265_SCRATCH ,
} tPP_MemType;

/*enum for decode mode*/
typedef enum
{
  PP_H265_DECODE_ACCESSUNIT = 0,
  PP_H265_PARSE_HEADER,
} tPP_DecodeMode;
/* enum for input low delay mode*/
typedef enum
{
  PP_H265_ENTIREINPUTFRAME = 0,
  PP_H265_SLICEMODE
} tPP_InputLowDelayMode;

/* enum for input low delay mode*/
typedef enum
{
  PP_H265_ENTIREOUTPUTFRAME = 0,
  PP_H265_NUMCTUROWS
} tPP_OutputLowDelayMode;

typedef enum
{
  PP_H265_METADATAPLANE_NONE = 0,
  PP_H265_METADATA_SEI_DATA,
  PP_H265_METADATA_VUI_DATA
} tPP_MetadataType;

/* SEI Scene Transition Type definition */
/**
 *  @brief SEI SceneTransitionType types
 */
typedef enum
{
  H265_SEI_STT_NO_TRASITION = 0,
  H265_SEI_STT_FADE_TO_BLACK,
  H265_SEI_STT_FADE_FROM_BLACK,
  H265_SEI_STT_UNSPECIFIED_TRASITION,
  H265_SEI_STT_DISSOLVE,
  H265_SEI_STT_WIPE,
  H265_SEI_STT_UNSPECIFIED_MIXTURE
} H265_SEI_SceneTransitionType;

/* SEI Tone Mapping Info Model Id related definition */
/**
 *  @brief SEI Tone Mapping Info Model Id types
 */
typedef enum
{
  H265_SEI_TMI_LINEAR_MAPPING = 0,
  H265_SEI_TMI_SIGMOIDAL_MAPPING,
  H265_SEI_TMI_USER_DEFINED_TABLE_MAPPING,
  H265_SEI_TMI_PIECE_WISE_LINEAR_MAPPING,
  H265_SEI_TMI_LUMINANCE_DYNAMIC_RANGE_INFO,
  H265_SEI_TMI_RESERVED
}H265_SEI_ToneMappingInfo_ModelId;

/* SEI Frame Packing Arrangement Type related definition */
/**
 *  @brief SEI Frame Packing Arrangement Type
 */
typedef enum
{
  H265_SEI_FPA_SIDE_BYSIDE = 3,
  H265_SEI_FPA_TOP_BOTTOM,
  H265_SEI_FPA_TEMPORAL_INTERLEAVING
}H265_SEI_FramePackingArrangementType;

/* SEI Decode Picture Hash Type related definition */
/**
 *  @brief SEI Decode Picture Hash Type
 */
typedef enum
{
  H265_SEI_DPHT_MD5 = 0,
  H265_SEI_DPHT_CRC,
  H265_SEI_DPHT_CHECKSUM
}H265_SEI_DecodePicHashType;

/**
 *  @brief Structure for H265_SeiBufferingPeriod parameters
 *
 */
typedef struct H265_SeiBufferingPeriod tH265_SeiBufferingPeriod;
struct H265_SeiBufferingPeriod
{
  /* SeiBufferingPeriod Present Flag*/
  tPPBool        nSeiBufferingPeriodPresentFlag;
  /* bp_seq_parameter_set_id */
  tPPu32      nBpSeqParamSetId;
  /* irap_cpb_params_present_flag */
  tPPu32      nIrapCpbParamsPresentFlag;
  /* cpb_delay_offset */
  tPPu32      nCpbDelayOffset;
  /* dpb_delay_offset */
  tPPu32      nDpbDelayOffset;
  /* concatenation_flag */
  tPPu32      nConcatenationFlag;
  /* au_cpb_removal_delay_delta_minus1 */
  tPPu32      nAuCpbRemovalDelayDeltaMinus1;
  /* nal_initial_cpb_removal_delay[]*/
  tPPu32      nNalInitialCpbRemovalDelay[H265_MAX_CPB_CNT];
  /*nal_initial_alt_cpb_removal_delay[]*/
  tPPu32      nNalInitialAltCpbRemovalDelay[H265_MAX_CPB_CNT];
  /* nal_initial_cpb_removal_offset[] */
  tPPu32      nNalInitialCpbRemovalOffset[H265_MAX_CPB_CNT];
  /*nal_initial_alt_cpb_removal_offset[]*/
  tPPu32      nNalInitialAltCpbRemovalOffset[H265_MAX_CPB_CNT];
  /*vcl_initial_cpb_removal_delay[]*/
  tPPu32      nVclInitialCpbRemovalDelay[H265_MAX_CPB_CNT];
  /*vcl_initial_alt_cpb_removal_delay[]*/
  tPPu32      nVclInitialAltCpbRemovalDelay[H265_MAX_CPB_CNT];
  /*vcl_initial_cpb_removal_offset[]*/
  tPPu32      nVclInitialCpbRemovalOffset[H265_MAX_CPB_CNT];
  /* vcl_initial_alt_cpb_removal_offset[]*/
  tPPu32      nVclInitialAltCpbRemovalOffset[H265_MAX_CPB_CNT];
};


/**
 *  @brief Structure for H265_SeiPanScanRect parameters
 *
 */
typedef struct H265_SeiPanScanRect tH265_SeiPanScanRect;
struct H265_SeiPanScanRect
{
  /* SeiPanScanRect Present Flag*/
  tPPBool        nSeiPanScanRectPresentFlag;
   /* pan_scan_rect_id */
  tPPu32      nPanScanRectId;
   /* pan_scan_rect_cancel_flag */
  tPPu32      nPanScanRectCancelFlag ;
  /* pan_scan_cnt_minus1 */
  tPPu32      nPanScanCntMinus1;
  /* pan_scan_rect_left_offset[ index ] */
  tPPi32      nPanScanRectLeftOffset[H265_MAX_PAN_SCAN_RECT];
   /* pan_scan_rect_right_offset[ index ] */
  tPPi32      nPanScanRectRightOffset[H265_MAX_PAN_SCAN_RECT];
  /* pan_scan_rect_top_offset[ index ] */
  tPPi32      nPanScanRectTopOffset[H265_MAX_PAN_SCAN_RECT];
   /* pan_scan_rect_bottom_offset[ index ] */
  tPPi32      nPanScanRectBottomOffset[H265_MAX_PAN_SCAN_RECT];
  /* added pan_scan_rect_persistence_flag */
  tPPu32      nPanScanRectPersistenceFlag;
};
/**
 *  @brief Structure for H265_SeiPictureTiming parameters
 *
 */
typedef struct H265_SeiPictureTiming tH265_SeiPictureTiming;
struct H265_SeiPictureTiming
{
  /* SeiPictureTiming Present Flag */
  tPPBool        nSeiPictureTimingPresentFlag;
  /* pic_struct */
  tPPu8      nPicStruct;
  /* source_scan_type */
  tPPu8      nSourceScanType;
  /*duplicate_flag*/
  tPPu8      nDuplicateFlag;
  /* au_cpb_removal_delay_minus1 */
  tPPu32     nAuCpbRemovalDelayMinus1;
  /* pic_dpb_output_delay */
  tPPu32     nPicDpbOutputDelay;
  /* pic_dpb_output_du_delay */
  tPPu32     nPicDpbOutputDuDelay;
  /* num_decoding_units_minus1 */
  tPPu32     nNumDecodingUnitsMinus1;
  /* du_common_cpb_removal_delay_flag */
  tPPu32     nDuCommonCpbRemovalDelayFlag;
  /* du_common_cpb_removal_delay_increment_minus1 */
  tPPu32     nDuCommonCpbRemovalDelayIncrementMinus1;
  /* num_nalus_in_du_minus1[ i ] */
  tPPu32      nNumNalusInDuMinus1[PP_H265_MAX_NUM_LCU] ;
  /* du_cpb_removal_delay_increment_minus1[ i ] */
  tPPu32      nDuCpbRemovalDelayIncrementMinus1[PP_H265_MAX_NUM_LCU];
};
/**
 *  @brief Structure for H265_SeiFillerPayload parameters
 *
 */
typedef struct H265_SeiFillerPayload  tH265_SeiFillerPayload;
struct H265_SeiFillerPayload
{
  /* SeiFillerPayload Present Flag */
  tPPBool        nSeiFillerPayloadPresentFlag;
  tPPu8       nFillerPayload;
};
/**
 *  @brief Structure for H265_SeiUserDataRegistered parameters
 *
 */
typedef struct H265_SeiUserDataRegistered tH265_SeiUserDataRegistered;
struct H265_SeiUserDataRegistered
{
   /* SeiUserDataRegistered Present Flag*/
  tPPBool        nSeiUserDataRegisteredPresentFlag;
  /* itu_t_t35_country_code */
  tPPu32      nItuTt35CountryCode;
   /* itu_t_t35_country_code_extension_byte */
  tPPu32      nItuTt35CountryCodeExtensionByte ;
  /* itu_t_t35_payload_byte */
  tPPu32      nItuTt35PayloadByte;
};

/**
 *  @brief Structure for H265_SeiUserDataUnregistered parameters
 *
 */
typedef struct H265_SeiUserDataUnregistered tH265_SeiUserDataUnregistered;
struct H265_SeiUserDataUnregistered
{
  /* SeiUserDataUnregistered Present Flag */
  tPPBool        nSeiUserDataUnregisteredPresentFlag;
  /* uuid_iso_iec_11578 */
  tPPu8       nUuid_iso_iec_11578[H265_SEI_UUID_ISO_IEC_128];
  /* user_data_payload_byte */
  tPPu8       nUserDataPayloadbyte[H265_MAX_SEI_USERDATA];
};

/**
 *  @brief Structure for H265_SeiRecoveryPoint parameters
 *
 */
typedef struct H265_SeiRecoveryPoint tH265_SeiRecoveryPoint;
struct H265_SeiRecoveryPoint
{
  /* SeiRecoveryPoint Present Flag */
  tPPBool        nSeiRecoveryPointPresentFlag;
  /* recovery_poc_cnt */
  tPPi32       nRecoveryPocCnt;
  /* exact_match_flag */
  tPPu32       nExactMatchFlag;
  /* broken_link_flag */
  tPPu32       nBrokenLinkFlag;
};

/**
 *  @brief Structure for H265_SeiSceneInfo parameters
 *
 */
typedef struct H265_SeiSceneInfo tH265_SeiSceneInfo;
struct H265_SeiSceneInfo
{
  /* SeiSceneInfo Present Flag */
  tPPBool        nSeiSceneInfoPresentFlag;
   /* scene_info_present_flag */
  tPPu32       nSceneInfoPresentFlag;
  /* prev_scene_id_valid_flag */
  tPPu32       nPrevSceneIdValidFlag;
  /* scene_id */
  tPPu32       nSceneId;
  /* scene_transition_type */
  tPPu32       nSceneTransitionType;
  /* second_scene_id */
  tPPu32       nSecondSceneId;
};

/**
 *  @brief Structure for H265_SeiPictureSnapShot parameters
 *
 */
typedef struct H265_SeiPictureSnapShot tH265_SeiPictureSnapShot;
struct H265_SeiPictureSnapShot
{
  /* SeiPictureSnapShot Present Flag */
  tPPBool        nSeiPictureSnapShotPresentFlag;
  /* snapshot_id */
  tPPu32      nSnapshotId;
};

/**
 *  @brief Structure for H265_SeiProgRefinementStart parameters
 *
 */
typedef struct H265_SeiProgRefinementStart\
               tH265_SeiProgRefinementStart;
struct H265_SeiProgRefinementStart
{
  /* SeiProgRefinementStart Present Flag */
  tPPBool        nSeiProgRefinementStartPresentFlag;
  /* progressive_refinement_id */
  tPPu32       nProgressiveRefinementId;
   /* pic_order_cnt_delta */
  tPPu32       nPicOrderCntDelta;
};

/**
 *  @brief Structure for H265_SeiProgRefinementEnd parameters
 *
 */
typedef struct H265_SeiProgRefinementEnd tH265_SeiProgRefinementEnd;
struct H265_SeiProgRefinementEnd
{
  /* SeiProgRefinementEnd Present Flag */
  tPPBool         nSeiProgRefinementEndPresentFlag;
   /* progressive_refinement_id */
  tPPu32       nProgressiveRefinementId;
};

/**
 *  @brief Structure for H265_SeiFilmGrainChar parameters
 *
 */
typedef struct H265_SeiFilmGrainChar tH265_SeiFilmGrainChar;
struct H265_SeiFilmGrainChar
{
  /* SeiFilmGrainChar Present Flag */
  tPPBool        nSeiFilmGrainCharPresentFlag;
  /* film_grain_characteristics_cancel_flag */
  tPPu32       nFilmGrainCharacteristicsCancelFlag;
  /* model_id */
  tPPu32       nModelId;
  /* separate_colour_description_present_flag */
  tPPu32       nSeparateColourDescriptionPresentFlag;
  /* film_grain_bit_depth_luma_minus8 */
  tPPu32       nFilmGrainBitDepthLumaMinus8;
  /* film_grain_bit_depth_chroma_minus8 */
  tPPu32       nFilmGrainBitDepthChromaMinus8;
  /* film_grain_full_range_flag */
  tPPu32       nFilmGrainFullRangeFlag;
   /* film_grain_colour_primaries */
  tPPu32       nFilmGrainColourPrimaries;
  /* film_grain_transfer_characteristics */
  tPPu32       nFilmGrainTransferCharacteristics;
  /* film_grain_matrix_coeffs */
  tPPu32       nFilmGrainMatrixCoefficients;
  /* blending_mode_id added */
  tPPu32       nBlendingModeId;
  /* log2_scale_factor */
  tPPu32       nLog2ScaleFactor;
  /* comp_model_present_flag[ nIndex ] */
  tPPu32       nCompModelPresentFlag[H265_SEI_TOTAL_COL_COMP];
   /* num_intensity_intervals_minus1[ nIndex ] */
  tPPu32       nNumIntensityIntervalsMinus1[H265_SEI_TOTAL_COL_COMP];
  /* num_model_values_minus1[ nIndex ] */
  tPPu32       nNumModelValuesMinus1[H265_SEI_TOTAL_COL_COMP];
  /* intensity_interval_lower_bound[ nIndex ][ nIndex1 ] */
  tPPu32       nIntensityIntervallowerBound[H265_SEI_TOTAL_COL_COMP]\
                      [H265_SEI_INTENSITY_LEVEL_INTERVAL];
   /* intensity_interval_upper_bound[ nIndex ][ nIndex1 ] */
  tPPu32       nIntensityIntervalUpperBound[H265_SEI_TOTAL_COL_COMP]\
                      [H265_SEI_INTENSITY_LEVEL_INTERVAL];
  /* comp_model_value[ nIndex ][ nIndex1 ][ nIndex2 ] */
  tPPu32       nCompModelValue[H265_SEI_TOTAL_COL_COMP]\
                      [H265_SEI_INTENSITY_LEVEL_INTERVAL]\
                      [H265_SEI_COL_COMP_MAX_MODEL_ID];
  /* film_grain_characteristics_persistence_flag added */
  tPPi32        nFilmGrainCharacteristicsPersistenceFlag;
};

/**
 *  @brief Structure for H265_SeiPostFilterHint parameters
 *
 */
typedef struct H265_SeiPostFilterHint tH265_SeiPostFilterHint;
struct H265_SeiPostFilterHint
{
  /* SeiPostFilterHin Present Flag */
  tPPBool         nSeiPostFilterHinPresentFlag;
  /* filter_hint_size_y */
  tPPu32       nFilterHintSizeY;
  /* filter_hint_size_x */
  tPPu32       nFilterHintSizeX;
   /* filter_hint_type */
  tPPu32       nFilterHintType;
  /* filter_hint[ nIndex ][ nIndex1 ][ nIndex2 ] */
  tPPi32        nFilterHint[H265_SEI_TOTAL_COL_COMP]\
                     [H265_SEI_COL_FILTER_HINT_Y][H265_SEI_COL_FILTER_HINT_X];
};

/**
 *  @brief Structure for H265_SeiToneMappingInfo parameters
 *
 */
typedef struct H265_SeiToneMappingInfo tH265_SeiToneMappingInfo;
struct H265_SeiToneMappingInfo
{
  /* SeiToneMappingInfo Present Flag */
  tPPBool         nSeiToneMappingInfoPresentFlag;
  /* tone_map_id */
  tPPu32       nToneMapId ;
  /* tone_map_cancel_flag */
  tPPBool         nToneMapCancelFlag;
  /* tone_map_persistence_flag */
  tPPBool         nToneMapPersistenceFlag;
  /* coded_data_bit_depth */
  tPPu8        nCodedDataBitDepth;
  /* target_bit_depth */
  tPPu8        nTargetBitDepth;
  /* tone_map_model_id */
  tPPu32        nToneMapModelId;
  /* min_value */
  tPPu32       nMinValue;
  /* max_value */
  tPPu32       nMaxValue;
  /* sigmoid_midpoint */
  tPPu32       nSigmoidMidpoint;
  /* sigmoid_width */
  tPPu32       nSigmoidWidth;
  /* start_of_coded_interval[ i ] */
  tPPu32       nStartOfCodedInterval[ 1 << H265_SEI_BIT_DEPTH];
  /* num_pivots */
  tPPu16       nNumPivots;
  /* coded_pivot_value[ i ]  */
  tPPu32       nCodedPivotValue[H265_MAX_NUM_PIVOTS];
  /* target_pivot_value[ i ] */
  tPPu32       nTargetPivotValue[H265_MAX_NUM_PIVOTS];
   /* camera_iso_speed_idc */
  tPPu8        nCameraIsoSpeedIdc;
   /* camera_iso_speed_value */
  tPPu32       nCameraIsoSpeedValue;
   /* exposure_index_idc */
  tPPu8        nExposureIndexIdc;
   /* exposure_index_value */
  tPPu32       nExposureIndexValue;
   /* exposure_compensation_value_sign_flag */
  tPPBool         nExposureCompensationValueSignFlag;
   /* exposure_compensation_value_numerator */
  tPPu16       nExposureCompensationValueNumerator;
   /* exposure_compensation_value_denom_idc */
  tPPu16       nExposureCompensationValueDenomIdc;
   /* ref_screen_luminance_white */
  tPPu32       nRefScreenLuminanceWhite;
   /* extended_range_white_level */
  tPPu32       nExtendedRangeWhiteLevel;
   /* nominal_black_level_code_value */
  tPPu16       nNominalBlackLevelCodeValue;
   /* nominal_white_level_code_value */
  tPPu16       nNominalWhiteLevelCodeValue;
  /* extended_white_level_code_value u(16) */
  tPPu16       nExtendedWhiteLevelCodeValue;
};

/**
 *  @brief Structure for H265_SeiFramePackingArrange parameters
 *
 */
typedef struct H265_SeiFramePackingArrange\
               tH265_SeiFramePackingArrange;
struct H265_SeiFramePackingArrange
{
  /* Sei Frame Packing Arrange Present Flag*/
  tPPBool         nSeiFramePackingArrangePresentFlag;
  /* frame_packing_arrangement_id */
  tPPu32       nFramePackingArrangementId;
  /* frame_packing_arrangement_cancel_flag */
  tPPu32       nFramePackingArrangementCancelFlag;
  /* frame_packing_arrangement_type */
  tPPu32       nFramePackingArrangementType;
  /* quincunx_sampling_flag */
  tPPu32       nQuincunxSamplingFlag;
  /* content_interpretation_type */
  tPPu32       nContentInterpretationType;
  /* spatial_flipping_flag */
  tPPu32       nSpatialFlippingFlag;
  /* frame0_flipped_flag */
  tPPu32       nFrame0FlippedFlag;
  /* field_views_flag */
  tPPu32       nFieldViewsFlag;
   /* current_frame_is_frame0_flag */
  tPPu32       nCurrentFrameIsFrame0Flag;
  /* frame0_self_contained_flag */
  tPPu32       nFrame0SelfContainedFlag;
  /* frame1_self_contained_flag */
  tPPu32       nFrame1SelfContainedFlag;
  /* frame0_grid_position_x */
  tPPu32       nFrame0GridPositionX;
  /* frame0_grid_position_y */
  tPPu32       nFrame0GridPositionY;
  /* frame1_grid_position_x */
  tPPu32       nFrame1GridPositionX;
   /* frame1_grid_position_y */
  tPPu32       nFrame1GridPositionY;
  /* frame_packing_arrangement_reserved_byte */
  tPPu32       nFramePackingArrangementReservedByte;
  /* frame_packing_arrangement_persistence_flag */
  tPPu32       nFramePackingArrangementRepetitionPeriod;
  /* upsampled_aspect_ratio_flag */
  tPPu32       nFramePackingArrangementExtensionFlag;
};

/**
 *  @brief Structure for H265_SeiDisplayOrientaiton parameters
 *
 */
typedef struct H265_SeiDisplayOrientaiton tH265_SeiDisplayOrientaiton;
struct H265_SeiDisplayOrientaiton
{
  /* Sei Display Orientaiton Present Flag */
  tPPBool        nSeiDisplayOrientaitonPresentFlag;
  /* display_orientation_cancel_flag */
  tPPu32       nDisplayOrientationCancelFlag ;
   /* hor_flip */
  tPPu32       nHorFlip;
   /* ver_flip */
  tPPu32       nVerFlip;
  /* anticlockwise_rotation */
  tPPu32       nAnticlockwiseRotation;
  /* display_orientation_persistence_flag */
  tPPu32       nDisplayOrientationRepetitionPeriod;
};

/**
 *  @brief Structure for H265_SeiStructureOfPictureInfo parameters
 *
 */
typedef struct H265_SeiStructureOfPictureInfo\
               tH265_SeiStructureOfPictureInfo;
struct H265_SeiStructureOfPictureInfo
{
  /* Sei Structure Of Picture Info Present Flag */
  tPPBool         nSeiStructureOfPictureInfoPresentFlag;
  /* sop_seq_parameter_set_id */
  tPPu32       nSpsId;
  /* num_entries_in_sop_minus1 */
  tPPu32       nNumPicsInSopMinus1;
  /* Not sure about the max_value. Now set to number of short term refs(65) */
  /* sop_vcl_nut[ index ] */
  tPPu32       nSopDescNalRefFlag[H265_SEI_MAX_NUM_SOP] ;
  /* sop_temporal_id[ index ] */
  tPPu32       nSopDescTemporalId[H265_SEI_MAX_NUM_SOP] ;
  /* sop_short_term_rps_idx[ index ] */
  tPPu32       nStRpsIdx[H265_SEI_MAX_NUM_SOP] ;
  /* sop_poc_delta[ index ] */
  tPPi32        nPocDelta[H265_SEI_MAX_NUM_SOP] ;
};

/**
 *  @brief Structure for H265_SeiDecodedPictureHash parameters
 *
 */
typedef struct H265_SeiDecodedPictureHash \
               tH265_SeiDecodedPictureHash;
struct H265_SeiDecodedPictureHash
{
  /* Sei Decoded Picture Hash Present Flag */
  tPPBool         nSeiDecodedPictureHashPresentFlag;
  /* hash_type */
  tPPu32       nHashType ;
  /* picture_md5[ index ][ index1 ] */
  tPPu32       nPictureMd5[H265_SEI_TOTAL_COL_COMP]\
                      [H265_SEI_MAX_MD5_DATA_LEN] ;
  /* picture_crc[ index ] */
  tPPu32       nPictureCrc[H265_SEI_TOTAL_COL_COMP];
  /* picture_checksum[ index ] */
  tPPu32       nPictureChecksum[H265_SEI_TOTAL_COL_COMP];
};

/**
 *  @brief Structure for H265_SeiActiveParameterSets parameters
 *
 */
typedef struct H265_SeiActiveParameterSets\
               tH265_SeiActiveParameterSets;
struct H265_SeiActiveParameterSets
{
  /* Sei Active Parameter Sets Present Flag */
  tPPBool         nSeiActiveParameterSetsPresentFlag;
  /* active_video_parameter_set_id */
  tPPu32       nActiveVpsId;
  /* self_contained_cvs_flag */
  tPPu32       nSelfContainedCSVFlag;
  /* no_parameter_set_update_flag */
  tPPu32       nNumParameterSetUpdateFlag;
   /* num_sps_ids_minus1 */
  tPPu32       nNumSpsIdsMinus1;
  /* active_seq_parameter_set_id[ i ] */
  tPPu32       nActiveSeqParameterSetId[PP_H265VDEC_MAX_REF_FRAMES];
};

/**
 *  @brief Structure for H265_SeiTemporalSubLayerZeroIndex parameters
 *
 */
typedef struct H265_SeiTemporalSubLayerZeroIndex
                 tH265_SeiTemporalSubLayerZeroIndex;
struct H265_SeiTemporalSubLayerZeroIndex
{
  /* Sei Temporal Sub-Layer Zero Index Present Flag */
  tPPBool        nSeiTemporalSubLayerZeroIndexPresentFlag;
   /* temporal_sub_layer_zero_idx */
  tPPu8       nTemporalSubLayerZeroIdx;
  /* irap_pic_id */
  tPPu8       nIrapPicId;
};

/**
 *  @brief Structure for H265_SEIDecodingUnitInfo parameters
 *
 */
typedef struct H265_SEIDecodingUnitInfo tH265_SEIDecodingUnitInfo;
struct H265_SEIDecodingUnitInfo
{
  /* Sei Decoding Unit Info Present Flag */
  tPPBool        nSEIDecodingUnitInfoPresentFlag;
  /* decoding_unit_idx */
  tPPu32       nDecodingUnitIdx;
  /* du_spt_cpb_removal_delay_increment */
  tPPu32       nDuSptCpbRemovalDelayIncrement;
  /* dpb_output_du_delay_present_flag */
  tPPu32       nDpbOutputDuDelayPresentFlag;
  /* pic_spt_dpb_output_du_delay_flag */
  tPPu32       nPicSptDpbOutputDuDelayFlag;
};

/**
 *  @brief Structure for H265_SEIScalableNesting parameters
 *
 */
typedef struct H265_SEIScalableNesting tH265_SEIScalableNesting;
struct H265_SEIScalableNesting
{
  /* Sei Scalable Nesting Present Flag */
  tPPBool        nSEIScalableNestingPresentFlag;
  /* bitstream_subset_flag */
  tPPu8       nBitstreamSubsetFlag;
  /* nesting_op_flag */
  tPPu8       nNestingOpFlag;
  /* default_op_flag */
  tPPu8       nDefaultOpFlag;
  /* nesting_num_ops_minus1 */
  tPPu32       nNestingNumOpsMinus1;
  /* nesting_max_temporal_id_plus1[ i ] */
  tPPu8       nNestingMaxTemporalIdPlus1[ H265_SEI_MAX_NUM_NESTING_OPS ];
  /* nesting_op_idx[ i ] */
  tPPu32       nNestingOpIdx[ H265_SEI_MAX_NUM_NESTING_OPS ];
  /* all_layers_flag */
  tPPu8       nAllLayersFlag;
  /* nesting_no_op_max_temporal_id_plus1 */
  tPPu8       nNestingNoOpMaxTemporalIdPlus1;
  /* nesting_num_layers_minus1 */
  tPPu8       nNestingNumLayersMinus1;
  /* nesting_layer_id[ i ] */
  tPPu8       nNestingLayerId[ H265_SEI_MAX_NUM_NESTING_LAYER ];
  /* nesting_zero_bit */
  tPPu8       nNestingZeroBit;
};

/**
 *  @brief Structure for H265_SEIRegionRefreshInfo parameters
 *
 */
typedef struct H265_SEIRegionRefreshInfo tH265_SEIRegionRefreshInfo;
struct H265_SEIRegionRefreshInfo
{
   /* Sei Region Refresh Info Present Flag */
  tPPBool        nSEIRegionRefreshInfoPresentFlag;
  /* refreshed_region_flag */
  tPPu8       nRefreshedRegionFlag;
};

/**
 *  @brief Structure for H265_SEISeiReserved parameters
 *
 */
typedef struct H265_SEISeiReserved tH265_SEISeiReserved;
struct H265_SEISeiReserved
{
  /*
   * reserved for future backward-compatible
   * parameter mentoind below is not the actual one
   */
   /* Sei SEI Reserved Present Flag */
  tPPBool        nSeiReservedPresentFlag;
  /* reserved_sei_message_payload_byte */
  tPPu8       nReservedSeiMessagePayloadByte;
};
/**
 *  @brief Structure for H265_SubLayerHRD Parameters
 *
 */
typedef struct H265_SubLayerHRDParameters\
               tH265_SubLayerHRDParameters;
struct H265_SubLayerHRDParameters
{
  /* bit_rate_value_minus1[ 32 ] */
  tPPu32   nBitRateValueMinus1[ H265_HRD_MAX_CPB_COUNT ];
  /* cpb_size_value_minus1[ 32 ] */
  tPPu32   nCpbSizeValueMinus1[ H265_HRD_MAX_CPB_COUNT ];
  /* cpb_size_du_value_minus1[ 32 ] */
  tPPu32   nCpbSizeDuValueMinus1[ H265_HRD_MAX_CPB_COUNT ];
  /** bit_rate_du_value_minus1[ 32 ] */
  tPPu32   nBitRateDuValueMinus1[ H265_HRD_MAX_CPB_COUNT ];
  /* cbr_flag[ i ] */
  tPPBool     nCbrFlag[ H265_HRD_MAX_CPB_COUNT ];
};
/**
 *  @brief Structure for H265_HRD Parameters
 *
 */
typedef struct H265_HRDParameters tH265_HRDParameters;
struct H265_HRDParameters
{
  /* HRD param valid flag */
  tPPBool     nHrdParametersValidFlag;
  /* nal_hrd_parameters_present_flag */
  tPPBool     nNalHrdParametersPresentFlag;
  /* vcl_hrd_parameters_present_flag */
  tPPBool     nVclHrdParametersPresentFlag;
  /* sub_pic_cpb_params_present_flag */
  tPPBool     nSubPicCpbParamsPresentFlag;
  /* tick_divisor_minus2 */
  tPPu8    nTickDivisorMinus2;
  /* du_cpb_removal_delay_increment_length_minus1 */
  tPPu8    nDuCpbRemovalDelayIncrementLengthMinus1;
  /* sub_pic_cpb_params_in_pic_timing_sei_flag */
  tPPu8    nSubPicCpbParamsInPicTimingSeiFlag;
  /* dpb_output_delay_du_length_minus1 */
  tPPu8    nDpbOutputDelayDuLengthMinus1;
  /* bit_rate_scale */
  tPPu8    nBitRateScale;
  /* cpb_size_scale */
  tPPu8    nCpbSizeScale;
  /* cpb_size_du_scale */
  tPPu8    nCpbSizeDuScale;
  /* initial_cpb_removal_delay_length_minus1 */
  tPPu8    nInitialCpbRemovalDelayLengthMinus1;
  /* au_cpb_removal_delay_length_minus1 */
  tPPu8    nAuCpbRemovalDelayLengthMinus1;
  /* dpb_output_delay_length_minus1 */
  tPPu8    nDpbOutputDelayLengthMinus1;
  /* fixed_pic_rate_general_flag[ 6 ] */
  tPPBool     nFixedPicRateGeneralFlag[ H265_HRD_MAX_NUM_SUB_LAYER ];
  /* fixed_pic_rate_within_cvs_flag[ 6 ] */
  tPPBool     nFixedPicRateWithinCvsFlag[ H265_HRD_MAX_NUM_SUB_LAYER ];
  /* elemental_duration_in_tc_minus1[ 6 ] */
  tPPu16   nElementalDurationInTcMinus1[ H265_HRD_MAX_NUM_SUB_LAYER ];
  /* low_delay_hrd_flag[ 6 ] */
  tPPBool     nLowDelayHrdFlag[ H265_HRD_MAX_NUM_SUB_LAYER ];
  /* cpb_cnt_minus1[ 6 ] */
  tPPu8    nCpbCntMinus1[ H265_HRD_MAX_NUM_SUB_LAYER ];
   /* structure of H265_SubLayerHRDParameters */
  tH265_SubLayerHRDParameters nSubLayerHRDParameters\
    [H265_HRD_MAX_NUM_SUB_LAYER];
};

/**
 *  @brief Structure for IH265_VUI Parameters
 *
 */
typedef struct H265_VUIParameters tH265_VUIParameters;
struct H265_VUIParameters
{
  /* VUI param valid flag */
  tPPBool     nVUIParamsValidFlag;
  /* aspect_ratio_info_present_flag */
  tPPBool     nAspectRatioInfoPresentFlag;
  /* aspect_ratio_idc */
  tPPu8    nAspectRatioIdc;
  /* sar_widtht */
  tPPu16   nSarWidth;
  /* sar_height */
  tPPu16    nSarHeight;
  /* overscan_info_present_flag */
  tPPBool     nOverscanInfoPresentFlag;
  /* overscan_appropriate_flag */
  tPPBool     nOverScanAppropriateFlag;
  /* video_signal_type_present_flag */
  tPPBool     nVideoSignalTypePresentFlag;
  /* video_format */
  tPPu8    nVideoFormat;
  /* video_full_range_flag */
  tPPBool     nVideoFullRangeFlag;
  /* colour_description_present_flag */
  tPPBool     nColourDescriptionPresentFlag;
  /* colour_primaries */
  tPPu8    nColourPrimaries;
  /* transfer_characteristics */
  tPPu8    nTransferCharacteristics;
  /* matrix_coeffs */
  tPPu8    nMatrixCoeffs;
  /* chroma_loc_info_present_flag */
  tPPBool     nChromaLocInfoPresentFlag;
  /* chroma_sample_loc_type_top_field */
  tPPu8    nChromaSampleLocTypeTopField;
  /* chroma_sample_loc_type_bottom_field */
  tPPu8    nChromaSampleLocTypeBottomField;
  /* neutral_chroma_indication_flagld */
  tPPBool     nNeutralChromaIndicationFlagId;
  /* field_seq_flag */
  tPPBool     nFieldSeqFlag;
  /* frame_field_info_present_flag*/
  tPPBool     nFrameFieldInfoPresentFlag;
  /* default_display_window_flag */
  tPPBool     nDefaultDisplayWindowFlag;
  /* def_disp_win_left_offset */
  tPPu16   nDefDispWinLeftOffset;
  /* def_disp_win_right_offset */
  tPPu16   nDefDispWinRightOffset;
  /* def_disp_win_top_offset */
  tPPu16   nDefDispWinTopOffset;
  /* def_disp_win_bottom_offset */
  tPPu16   nDefDispWinBottomOffset;
  /* vui_timing_info_present_flag */
  tPPBool     nVUITimingInfoPresentFlag;
  /* vui_num_units_in_tick */
  tPPu32   nVUINumUnitsInTick;
  /* vui_time_scale */
  tPPu32   nVUITimeScale;
  /* vui_poc_proportional_to_timing_flag */
  tPPBool     nVUIpocProportionalToTimingFlag;
  /* vui_num_ticks_poc_diff_one_minus1 */
  tPPu32   nVUINumTicksPOCDiffOneMinus1;
  /* vui_hrd_parameters_present_flags */
  tPPBool     nVUIHrdParametersPresentFlags;
  /* bitstream_restriction_flag */
  tPPBool     nBitStreamRestrictionFlag;
  /* tiles_fixed_structure_flag */
  tPPBool     nTilesFixedStructureFlag;
  /* motion_vectors_over_pic_boundaries_flag*/
  tPPBool     nMotionVectorsOverPicBoundariesFlag;
  /* restricted_ref_pic_lists_flag*/
  tPPBool     nRestrictedRefPicListsFlag;
  /* min_spatial_segmentation_idc */
  tPPu16   nMinSpatialSegmentationIdc;
  /* max_bytes_per_pic_denom */
  tPPu8    nMaxBytesPerPicDenom;
  /* max_bits_per_min_cu_denom */
  tPPu8    nMaxBitsPerMinCuDenom;
  /* log2_max_mv_length_horizontal */
  tPPu8    nLog2MaxMvLengthHorizontal;
  /* log2_max_mv_length_vertical*/
  tPPu8    nLog2MaxMvLengthVertical;
  /* structure of H265_HRDParameters */
  tH265_HRDParameters nHrdParameters;
};
/**
 *  @brief Structure for H265_SeiMesseges parameters
 *
 */
typedef struct H265_SeiMesseges tH265_SeiMesseges;
struct H265_SeiMesseges
{
  /* structure of H265_SeiBufferingPeriod */
  tH265_SeiBufferingPeriod             nSeiBufferingPeriod;
   /* structure of */
  tH265_SeiPictureTiming               nSeiPictureTiming;
   /* structure of H265_SeiPictureTiming */
  tH265_SeiPanScanRect                 nSeiPanScanRect;
   /* structure of H265_SeiFillerPayload */
  tH265_SeiFillerPayload               nSeiFillerPayload;
   /* structure of H265_SeiUserDataRegistered */
  tH265_SeiUserDataRegistered          nSeiUserDataRegistered;
  /* structure of H265_SeiUserDataUnregistered */
  tH265_SeiUserDataUnregistered        nSeiUserDataUnregistered;
   /* structure of H265_SeiRecoveryPoint */
  tH265_SeiRecoveryPoint               nSeiRecoveryPoint;
   /* structure of H265_SeiSceneInfo */
  tH265_SeiSceneInfo                   nSeiSceneInfo;
   /* structure of H265_SeiPictureSnapShot */
  tH265_SeiPictureSnapShot             nSeiPictureSnapShot;
   /* structure of H265_SeiProgRefinementStart */
  tH265_SeiProgRefinementStart         nSeiProgRefinementStart;
   /* structure of H265_SeiProgRefinementEnd */
  tH265_SeiProgRefinementEnd           nSeiProgRefinementEnd;
   /* structure of H265_SeiFilmGrainChar */
  tH265_SeiFilmGrainChar               nSeiFilmGrainChar;
   /* structure of H265_SeiPostFilterHint */
  tH265_SeiPostFilterHint              nSeiPostFilterHint;
   /* structure of H265_SeiToneMappingInfo */
  tH265_SeiToneMappingInfo             nSeiToneMappingInfo;
   /* structure of H265_SeiFramePackingArrange */
  tH265_SeiFramePackingArrange         nSeiFramePackingArrange;
   /* structure of H265_SeiDisplayOrientaiton */
  tH265_SeiDisplayOrientaiton          nSeiDisplayOrientaiton;
   /* structure of H265_SeiStructureOfPictureInfo */
  tH265_SeiStructureOfPictureInfo      nSeiStructureOfPictureInfo;
   /* structure of H265_SeiActiveParameterSets */
  tH265_SeiActiveParameterSets         nSeiActiveParameterSets;
   /* structure of H265_SEIDecodingUnitInfo */
  tH265_SEIDecodingUnitInfo             nSeiDecodingUnitInfo;
   /* structure of H265_SeiTemporalSubLayerZeroIndex */
  tH265_SeiTemporalSubLayerZeroIndex   nSeiTemporalSubLayerZeroIndex;
   /* structure of H265_SeiDecodedPictureHash */
  tH265_SeiDecodedPictureHash          nSeiDecodedPictureHash;
   /* structure of H265_SEIScalableNesting */
  tH265_SEIScalableNesting              nSeiScalableNesting;
   /* structure of H265_SEIRegionRefreshInfo */
  tH265_SEIRegionRefreshInfo            nSeiRegionRefreshInfo;
   /* structure of H265_SEISeiReserved */
  tH265_SEISeiReserved                  nSeiReserved;
};


/**
 ******************************************************************************
 * @fn tPPResult gPP_H265_InitializeDecoder  (tPPBaseDecoder    **appBase,
 *                                       tPPQueryMemRecords  *apQueryMemRecords,
 *                                       tPP_H265_CreateParams *apCreateParams);
 * @ingroup H265_DECODER
 *
 * @brief This function initialises H.265 video decoder object
 *
 * @param appBase           [IN/OUT] Pointer to hold decoder object handle
 * @param apQueryMemRecords [IN] Structure holding allocated memory records
 * @param apCreateParams    [IN] Pointer to decoder create parameter structure
 *
 * @return None
 ******************************************************************************
 */
tPPResult gPP_H265_InitializeDecoder  (tPPBaseDecoder       **appBase,
                                  tPPQueryMemRecords  *apQueryMemRecords,
                                  tPP_H265_CreateParams *apCreateParams);


/**
 ******************************************************************************
 * @fn gPP_H265_QueryMemoryRequirements (tPPQueryMemRecords  *apQueryMemRecords,
 *                             tPPH265BPD_CreateParams *apCreateParams,
 *                             tPPu32 *apNumMemTabEntries);
 * @ingroup H265_DECODER
 *
 * @brief This function queries for H.265 video decoder object
 *              memory requirements.
 *
 * @param apQueryMemRecords  [IN/OUT] Pointer to memory query records
 * @param apCreateParams     [IN] Pointer to decoder create parameter structure
 * @param apNumMemTabEntries [OUT] Number of MemTab Entries
 *
 * @return None
 ******************************************************************************
 */

tPPResult gPP_H265_QueryMemoryRequirements(
                             tPPQueryMemRecords    *apQueryMemRecords,
                             tPP_H265_CreateParams *apCreateParams,
                             tPPu32                *apNumMemTabEntries);



/* ER codes */
/* enum for extended error used for bit-setting */
typedef enum
{
  PP_H265_ERR_UNSUPPORTED = 1,
  PP_H265_ERR_NO_NALU_BEGIN_FOUND,
  PP_H265_ERR_NO_NALU_END_FOUND,
  PP_H265_ERR_INVALID_NAL_UNIT_TYPE,
  PP_H265_ERR_INSUFFICIENT_BUFFER,
  PP_H265_ERR_DATA_SYNC,
  PP_H265_ERR_CRITICAL,
  PP_H265_ERR_NO_VPS,
  PP_H265_ERR_VPS,
  PP_H265_ERR_NO_SPS,
  PP_H265_ERR_SPS,
  PP_H265_ERR_NO_PPS,
  PP_H265_ERR_PPS,
  PP_H265_ERR_SLICELOSS,
  PP_H265_ERR_SLICEHDR,
  PP_H265_ERR_SLICEDATA,
  PP_H265_ERR_RANDOM_ACCESS_SKIP,
  PP_H265_ERR_REFPIC_NOT_FOUND,
  PP_H265_ERR_META_DATA
}PP_H265VDEC_Errors;

 /* Macro to set Error bits */
#define PP_H265_SET_ERROR_BIT(x, m)  ((x) |= (0x1 << m))

#ifdef __cplusplus
}
#endif

#endif /*_EXP_PP_H265DECODER_H_*/


