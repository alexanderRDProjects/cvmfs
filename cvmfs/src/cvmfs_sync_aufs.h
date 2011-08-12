#ifndef CVMFS_SYNC_AUFS_H
#define CVMFS_SYNC_AUFS_H

#include <string>
#include <set>



namespace cvmfs {
	enum FileType {FT_DIR, FT_REG, FT_SYM, FT_ERR};
	
	typedef struct {
		std::set<std::string> dir_add;
		std::set<std::string> dir_touch;
		std::set<std::string> dir_rem;
		std::set<std::string> reg_add;
		std::set<std::string> reg_touch;
		std::set<std::string> sym_add;
		std::set<std::string> fil_rem; ///< We don't know if this is regular file or symlink
	} Changeset;
	
	/**
	 *  abstract class for interface definition of repository sync based on
	 *  a union filesystem overlay over a mounted CVMFS volume.
	 */
	class UnionFilesystemSync {
	protected:
		std::string mRepositoryPath;
		std::string mOverlayPath;
		std::string mUnionPath;
		
		Changeset mChangeset;
		
	public:
		UnionFilesystemSync(const std::string &repositoryPath, const std::string &unionPath, const std::string &overlayPath);
		virtual ~UnionFilesystemSync();
		
		virtual bool goGetIt() = 0;
		
		Changeset getChangeset() const { return mChangeset; }
		
	protected:
		/**
		 *  checks if the given filename (without path) is interesting for sync
		 *  @param filename the filename to check
		 *  @return true if interesting otherwise false
		 */
		inline bool isInterestingFilename(const std::string &filename) { return not isIgnoredFilename(filename); }
		
		/**
		 *  checks if a filename can be ignored while reading out the overlay directory
		 *  @param filename the filename to check
		 *  @return true if the file should be ignored otherwise false
		 */ 
		virtual bool isIgnoredFilename(const std::string &filename) const = 0;
		
		/**
		 *  checks if given filename (without path) is supposed to be whiteout
		 *  these files show the union file system, that a specific file should appear as deleted
		 *  @param filename the filename to check
		 *  @return true if filename seems to be whiteout otherwise false
		 */
		virtual bool isWhiteoutFilename(const std::string &filename) const = 0;
		
		/**
		 *  retrieves the filename of the marked deleted file in the repository
		 *  @param filename the filename of the whiteout file
		 *  @return the filename of the 'deleted' file
		 */
		virtual std::string getFilenameFromWhiteout(const std::string &filename) const = 0;
		
		/**
		 *  checks if the given symbolic link points outside of the repository
		 *  if the link is an absolute path to the repository it will be transformed to
		 *  a relative path
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return true if link points to the repository otherwise false
		 */
		bool checkSymlink(const std::string &dirPath, const std::string &filename);
		
		/**
		 *  get the absolute path to a repository file
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return the absolute path to the given file in the repository
		 */
		std::string getPathToRepositoryFile(const std::string &dirPath, const std::string &filename) const;
		
		/**
		 *  get the absolute path to a file in the union filesystem overlay directory
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return the absolute path to the given file in the overlay directory
		 */
		std::string getPathToOverlayFile(const std::string &dirPath, const std::string &filename) const;
		
		/**
		 *  get the absolute path to a file in the union filesystem volume
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return the absolute path to the given file in the union filesystem volume
		 */
		std::string getPathToUnionFile(const std::string &dirPath, const std::string &filename) const;
		
		/**
		 *  retrieve the file type of a given file
		 *  @param path the path to the file to check
		 *  @return the file type of the file
		 */
		FileType getFileType(const std::string &path) const;
		
		/**
		 *  returns the type of a file/directory in the current repository
		 *  @param dirPath relative path of containing directory
		 *  @param filename the path to the file for lookup (relative to the aufs mounted directory)
		 *  @return the file type of the given file in the current repository
		 */
		FileType getFiletypeInRepository(const std::string &dirPath, const std::string &filename) const;
	
		/**
		 *  checks if the given file is present in the repository
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return true if the file is not present in the repository otherwise false
		 */
		virtual bool isNewItem(const std::string &dirPath, const std::string &filename) const;
		
		/**
		 *  checks the inodes of the given file to figure out, if the item was edited
		 *  or overwritten by another file
		 *  (virtual, because this may look differently in different union file systems)
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return true if the item was edited and false if it appears to be overwritten
		 */
		virtual bool isEditedItem(const std::string &dirPath, const std::string &filename) const = 0;

		/**
		 *  callback method for the main recursion when a regular file is found
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 */
		virtual void processFoundRegularFile(const std::string &dirPath, const std::string &filename);
		
		/**
		 *  callback method for the main recursion when a directory is found
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 *  @return true if recursion should dig into the given directory, false otherwise
		 */
		virtual bool processFoundDirectory(const std::string &dirPath, const std::string &filename);
		
		/**
		 *  callback method for the main recursion when a symlink is found
		 *  @param dirPath the relative directory path
		 *  @param filename the filename
		 */
		virtual void processFoundSymlink(const std::string &dirPath, const std::string &filename);
		
		/**
		 *  union file systems create files indicating, that a specific file of the read only volume
		 *  should appear as deleted. These files are called whiteout entries and are processed here
		 *  @param dirPath relative path to directory
		 *  @param filename name of the whiteout entry file
		 */
		virtual void processWhiteoutEntry(const std::string &dirPath, const std::string &filename);

		/**
		 *  recursively traverses the content of the given directory in the REPOSITORY (!)
		 *  for every found entry the according deletion method is called
		 *  @param dirPath the relative directory path
		 *  @param filename the directory name
		 */
		void deleteDirectoryRecursively(const std::string &dirPath, const std::string &filename);
		bool deleteDirectory(const std::string &dirPath, const std::string &filename);
		void deleteRegularFile(const std::string &dirPath, const std::string &filename);
		void deleteSymlink(const std::string &dirPath, const std::string &filename);

		/**
		 *  recursively traverses the content of the given directory and adds all
		 *  found entries by calling the according add method
		 *  @param dirPath the relative directory path
		 *  @param filename the directory name
		 */
		void addDirectoryRecursively(const std::string &dirPath, const std::string &filename);
		bool addDirectory(const std::string &dirPath, const std::string &filename);
		void touchDirectory(const std::string &dirPath, const std::string &filename);
		void addRegularFile(const std::string &dirPath, const std::string &filename);
		void touchRegularFile(const std::string &dirPath, const std::string &filename);
		void addSymlink(const std::string &dirPath, const std::string &filename);
		void touchSymlink(const std::string &dirPath, const std::string &filename);
		
		void printWarning(const std::string &warningMessage);
		void printError(const std::string &errorMessage);
	};
	
	/**
	 *  syncing a CVMFS repository by the help of an overlayed AUFS 1.x read-write volume
	 */
	class SyncAufs1 :
	 	public UnionFilesystemSync {
	private:
		std::set<std::string> mIgnoredFilenames;
		std::string mWhiteoutPrefix;
	
	public:
		SyncAufs1(const std::string &repositoryPath, const std::string &unionPath, const std::string &aufsPath);
		virtual ~SyncAufs1();
		
		bool goGetIt();
		
	protected:
		inline bool isWhiteoutFilename(const std::string &filename) const { return (filename.substr(0,mWhiteoutPrefix.length()) == mWhiteoutPrefix); }
		inline std::string getFilenameFromWhiteout(const std::string &filename) const { return filename.substr(mWhiteoutPrefix.length()); }
		
		inline bool isIgnoredFilename(const std::string &filename) const { return (mIgnoredFilenames.find(filename) != mIgnoredFilenames.end()); }
		virtual bool isEditedItem(const std::string &dirPath, const std::string &filename) const;
		
	private:
	};
	
	/**
	 *  @brief a simple recursion engine to abstract the recursion of directories
	 *  it provides different callback hooks to instrument and control the recursion
	 *  hooks will be called on the provided delegate object which has to be of type T
	 */
	template <class T>
	class RecursionEngine {
	private:
		/** the delegate all hooks are called on */
		T *mDelegate;
		
		/** dirPath in callbacks will be relative to this directory */
		std::string mRelativeToDirectory;
		
	public:
		/** message if a directory is entered by the recursion */
		void (T::*enteringDirectory)(const std::string &dirPath);
		
		/** message if a directory is left by the recursion */
		void (T::*leavingDirectory)(const std::string &dirPath);
		
		/** invoked to see if the delegate is interested in a found directory entry */
		bool (T::*caresAbout)(const std::string &filename);
		
		/** message if a file was found */
		void (T::*foundRegularFile)(const std::string &dirPath, const std::string &filename);
		
		/**
		 *  message if a directory was found
		 *  if the callback returns true, the recursion will continue in the found directory
		 *  otherwise it will skip it and continue with the next entry in to current director
		 */
		bool (T::*foundDirectory)(const std::string &dirPath, const std::string &filename);
		
		/** message if a link was found */
		void (T::*foundSymlink)(const std::string &dirPath, const std::string &filename);
		
	public:
		RecursionEngine(T *delegate, const std::string &relativeToDirectory);
		
		/**
		 *  start the recursion at the given directory
		 */
		void recurse(const std::string &dirPath) const;
		
	private:
		void doRecursion(const std::string &dirPath) const;
		
		std::string getRelativePath(const std::string &absolutePath) const;
	};
}

#endif /* CVMFS_SYNC_AUFS_H */
